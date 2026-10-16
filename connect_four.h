#ifndef CONNECT_FOUR_H
#define CONNECT_FOUR_H

#include <array>
#include <memory>
#include <random>
#include <vector>

#include "nnet/nn/sequential.h"


enum class Cell {
	Empty,
	PlayerOne,
	PlayerTwo
};


using Board = std::array<std::array<Cell, 7>, 6>;

struct moveRecord {
	int moveNumber;
	Cell player;
	Board boardBeforeMove;
	int chosenCol;
};

struct gameRecord {
	int gameId;
	std::vector<moveRecord> moves;
	Cell winnerAfterGame;
};


bool inBounds(int row, int col);

int countDirection(
	const Board& board, int row, int col, int rowStep, int colStep, Cell player);

Cell getWinner(const Board& board, int lastRow, int lastCol);

void printBoard(const Board& board);

bool columnFull(const Board& board, int col);

int dropPiece(Board& board, int col, Cell player);

bool boardFull(const Board& board);

Cell other(Cell player);

void appendBoard(std::vector<double>& inputValues, const Board& board, Cell aiPiece);

std::unique_ptr<nnet::Sequential> buildValueNetwork(std::mt19937& generator);

void trainAI(nnet::Sequential& model, const std::vector<gameRecord>& games, int epochs, double learningRate);

int chooseMove(nnet::Sequential& model, const Board& board, Cell player);

#endif
