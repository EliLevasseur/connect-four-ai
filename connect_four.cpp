#include "connect_four.h"
#include <memory>
#include <random>
#include <vector>

#include "nnet/core/tensor.h"
#include "nnet/core/tensor_ops.h"
#include "nnet/nn/dense.h"
#include "dataframe.h"
#include "nnet/optim/optimizer.h"
#include "nnet/core/autograd/operations.h"
# include "nnet/core/autograd/backward.h"
#include "nnet/nn/module.h"
#include "nnet/nn/sequential.h"
#include "nnet/train/trainer.h"
#include "nnet/nn/checkpoint.h"

#include <iostream>


bool inBounds(int row, int col) {
	return row >= 0 && row < 6 && col >= 0 && col < 7;
}


int countDirection(
	const Board& board, int row, int col, int rowStep, int colStep, Cell player) {
	int count = 0;

	row += rowStep;
	col += colStep;

	while (inBounds(row, col) &&
		   board[row][col] == player) {
		++count;
		row += rowStep;
		col += colStep;
	}

	return count;
}


Cell getWinner(const Board& board, int lastRow, int lastCol) {
	Cell player = board[lastRow][lastCol];

	if (player == Cell::Empty)
		return Cell::Empty;

	const int directions[4][2] = {
		{0, 1},
		{1, 0},
		{1, 1},
		{1, -1}
	};

	for (const auto& direction : directions) {
		int rowStep = direction[0];
		int colStep = direction[1];

		int total = 1;

		total += countDirection(
			board, lastRow, lastCol, rowStep, colStep, player);

		total += countDirection(board, lastRow, lastCol, -rowStep, -colStep, player);


		if (total >= 4)
			return player;
	}

	return Cell::Empty;
}

void printBoard(const Board& board) {
	std::cout << "\n  0 1 2 3 4 5 6\n";

	for (int row = 5; row >= 0; --row) {
		std::cout << row << " ";

		for (int col = 0; col < 7; ++col) {
			if (board[row][col] == Cell::Empty)
				std::cout << ".";
			else if (board[row][col] == Cell::PlayerOne)
				std::cout << "X";
			else
				std::cout << "O";

			std::cout << " ";
		}

		std::cout << "\n";
	}
}


bool columnFull(const Board& board, int col) {
	if (col < 0 || col >= 7)
		return true;

	return board[5][col] != Cell::Empty;
}


int dropPiece(Board& board, int col, Cell player) {
	if (columnFull(board, col))
		return -1;

	for (int row = 0; row < 6; ++row) {
		if (board[row][col] == Cell::Empty) {
			board[row][col] = player;
			return row;
		}
	}

	return -1;
}


bool boardFull(const Board& board) {
	for (int col = 0; col < 7; ++col) {
		if (!columnFull(board, col))
			return false;
	}

	return true;
}

Cell other(Cell player) {
	return player == Cell::PlayerOne ? Cell::PlayerTwo : Cell::PlayerOne;
}

// 1 if the player who made this move went on to win, 0.5 for a draw, 0 if they lost
double targetFor(Cell winner, Cell mover) {
	if (winner == Cell::Empty) return 0.5;
	return winner == mover ? 1.0 : 0.0;
}

void appendBoard(std::vector<double>& inputValues, const Board& board, Cell aiPiece) {
    for (const auto& boardRow : board)
        for (const Cell cell : boardRow)
            inputValues.push_back(cell == aiPiece ? 1.0 : 0.0);
    for (const auto& boardRow : board)
        for (const Cell cell : boardRow)
            inputValues.push_back(cell != aiPiece && cell != Cell::Empty ? 1.0 : 0.0);
}

// Parameters can't move, so the model is handed back inside a unique_ptr
std::unique_ptr<nnet::Sequential> buildValueNetwork(std::mt19937& generator) {
	std::vector<std::unique_ptr<nnet::Unary_Module>> components;
	components.push_back(std::make_unique<nnet::Dense>(84, 64, generator));
	components.push_back(std::make_unique<nnet::ReLU>());
	components.push_back(std::make_unique<nnet::Dense>(64, 1, generator));
	components.push_back(std::make_unique<nnet::Sigmoid>());
	return std::make_unique<nnet::Sequential>(std::move(components));
}

// trains the model in place on every position from every game
void trainAI(nnet::Sequential& model, const std::vector<gameRecord>& games, int epochs, double learningRate) {
	std::vector<double> inputValues;
	std::vector<double> targetValues;
	for (const gameRecord& game : games) {
		for (const moveRecord& move : game.moves) {
			appendBoard(inputValues, move.boardBeforeMove, move.player);
			targetValues.push_back(targetFor(game.winnerAfterGame, move.player));
		}
	}

	const std::size_t positionCount = targetValues.size();
	const nnet::Tensor inputs({positionCount, 84}, inputValues);
	const nnet::Tensor targets({positionCount, 1}, targetValues);

	nnet::AdamOptimizer optimizer(model.parameters(), learningRate);
	std::mt19937 shuffleGenerator(42);
	for (int epoch = 0; epoch < epochs; ++epoch) {
		nnet::trainEpoch(model, inputs, targets, 64, optimizer, nnet::binaryCrossEntropy, shuffleGenerator);
	}
}

// the network's guess at how likely `player` is to win, with `player` about to move
double chanceToWin(nnet::Sequential& model, const Board& board, Cell player) {
	std::vector<double> encoded;
	appendBoard(encoded, board, player);
	nnet::NoGrad noGrad;
	return model.forward(nnet::makeLeaf(nnet::Tensor({1, 84}, encoded)))->data.at({0, 0});
}

// take a win if there is one, otherwise leave the opponent the position
// the network thinks is worst for them
int chooseMove(nnet::Sequential& model, const Board& board, Cell player) {
	int bestColumn = -1;
	double bestScore = -1.0;
	for (int col = 0; col < 7; ++col) {
		if (columnFull(board, col)) continue;
		Board next = board;
		const int row = dropPiece(next, col, player);
		if (getWinner(next, row, col) == player) return col;
		const double score = 1.0 - chanceToWin(model, next, other(player));
		if (score > bestScore) {
			bestScore = score;
			bestColumn = col;
		}
	}
	return bestColumn;
}
