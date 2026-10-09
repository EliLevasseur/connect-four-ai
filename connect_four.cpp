#include "connect_four.h"

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
