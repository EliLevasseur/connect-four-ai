#ifndef CONNECT_FOUR_H
#define CONNECT_FOUR_H

#include <array>


enum class Cell {
	Empty,
	PlayerOne,
	PlayerTwo
};


using Board = std::array<std::array<Cell, 7>, 6>;


bool inBounds(int row, int col);

int countDirection(
	const Board& board, int row, int col, int rowStep, int colStep, Cell player);

Cell getWinner(const Board& board, int lastRow, int lastCol);

void printBoard(const Board& board);

bool columnFull(const Board& board, int col);

int dropPiece(Board& board, int col, Cell player);

bool boardFull(const Board& board);

#endif
