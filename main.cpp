#include "connect_four.h"

#include <iostream>


int main() {
	Board board;

	for (auto& row : board) {
		row.fill(Cell::Empty);
	}

	bool flag = true;
	int row;
	int col;

	printBoard(board);

	while (flag) {
		std::cout << "PLAYER ONE:\n";
		std::cout << "Enter a column (0-6) 100 to quit: ";
		std::cin >> col;

		if (col == 100)
			break;

		while (columnFull(board, col)) {
			if (col < 0 || col >= 7)
				std::cout << "Enter a column from 0 to 6: ";
			else
				std::cout << "That column is full. Enter another column: ";

			std::cin >> col;

			if (col == 100) {
				flag = false;
				break;
			}
		}

		if (!flag)
			break;

		row = dropPiece(board, col, Cell::PlayerOne);
		printBoard(board);

		if (getWinner(board, row, col) == Cell::PlayerOne) {
			std::cout << "PLAYER ONE WINS!\n";
			break;
		}

		if (boardFull(board)) {
			std::cout << "The game is a draw!\n";
			break;
		}

		std::cout << "\nPLAYER TWO:\n";
		std::cout << "Enter a column (0-6) 100 to quit: ";
		std::cin >> col;

		if (col == 100)
			break;

		while (columnFull(board, col)) {
			if (col < 0 || col >= 7)
				std::cout << "Enter a column from 0 to 6: ";
			else
				std::cout << "That column is full. Enter another column: ";

			std::cin >> col;

			if (col == 100) {
				flag = false;
				break;
			}
		}

		if (!flag)
			break;

		row = dropPiece(board, col, Cell::PlayerTwo);
		printBoard(board);

		if (getWinner(board, row, col) == Cell::PlayerTwo) {
			std::cout << "PLAYER TWO WINS!\n";
			break;
		}

		if (boardFull(board)) {
			std::cout << "The game is a draw!\n";
			break;
		}
	}

	return 0;
}
