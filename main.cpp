#include "connect_four.h"
#include "nnet/nn/checkpoint.h"

#include <iostream>
#include <string>


int main(int argc, char* argv[]) {
	// play against any saved generation: connect4 build/connect4/gen_3.ckpt
	const std::string checkpoint = argc > 1 ? argv[1] : "build/connect4/best.ckpt";
	std::mt19937 generator(42);
	auto model = buildValueNetwork(generator);
	try {
		nnet::loadParameters(*model, checkpoint);
	} catch (const std::exception& error) {
		std::cout << error.what() << "\nRun connect4_simulations first to train the AI.\n";
		return 1;
	}
	std::cout << "Playing against " << checkpoint << "\n";

	Board board;

	for (auto& row : board) {
		row.fill(Cell::Empty);
	}

	bool flag = true;
	int row;
	int col;

	// everything the AI needs to learn from this game afterwards
	gameRecord game{0, {}, Cell::Empty};
	int moveNumber = 0;
	bool gameFinished = false;

	std::vector<std::size_t> networkShape = {84, 128, 64, 1};
	

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

		game.moves.push_back({moveNumber++, Cell::PlayerOne, board, col});
		row = dropPiece(board, col, Cell::PlayerOne);
		printBoard(board);

		if (getWinner(board, row, col) == Cell::PlayerOne) {
			std::cout << "PLAYER ONE WINS!\n";
			game.winnerAfterGame = Cell::PlayerOne;
			gameFinished = true;
			break;
		}

		if (boardFull(board)) {
			std::cout << "The game is a draw!\n";
			gameFinished = true;
			break;
		}

		col = chooseMove(*model, board, Cell::PlayerTwo);
		std::cout << "\nAI plays column " << col << "\n";

		game.moves.push_back({moveNumber++, Cell::PlayerTwo, board, col});
		row = dropPiece(board, col, Cell::PlayerTwo);
		printBoard(board);

		if (getWinner(board, row, col) == Cell::PlayerTwo) {
			std::cout << "THE AI WINS!\n";
			game.winnerAfterGame = Cell::PlayerTwo;
			gameFinished = true;
			break;
		}

		if (boardFull(board)) {
			std::cout << "The game is a draw!\n";
			gameFinished = true;
			break;
		}
	}

	// a game you quit early has no result to learn from
	if (gameFinished) {
		trainAI(*model, {game}, 2, 0.005);
		nnet::saveParameters(*model, checkpoint);
		std::cout << "\nThe AI learned from this game and saved itself to " << checkpoint << "\n";
	}

	return 0;
}
