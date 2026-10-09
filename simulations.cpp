#include <vector>
#include <random>
#include <iostream>

#include "connect_four.h"

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

gameRecord gameSim(int gameId, std::mt19937& generator) {
	gameRecord record;
	record.gameId = gameId;
	record.winnerAfterGame = Cell::Empty;

	Board simBoard;

	for (auto& row : simBoard) {
		row.fill(Cell::Empty);
	}

	Cell player = Cell::PlayerOne;
	int moveNumber = 0;

	while (true) {
		std::vector<int> legalColumns;

		for (int col = 0; col < 7; col++) {
			if (!columnFull(simBoard, col))
				legalColumns.push_back(col);
		}

		if (legalColumns.empty())
			break;

		std::uniform_int_distribution<int> distribution(
			0, static_cast<int>(legalColumns.size()) - 1);

		int chosenCol = legalColumns[distribution(generator)];
		Board boardBeforeMove = simBoard;
		int row = dropPiece(simBoard, chosenCol, player);

		if (row == -1)
			break;

		record.moves.push_back({
			moveNumber,
			player,
			boardBeforeMove,
			chosenCol
		});

		Cell winner = getWinner(simBoard, row, chosenCol);

		if (winner != Cell::Empty) {
			record.winnerAfterGame = winner;
			break;
		}

		if (boardFull(simBoard))
			break;

		if (player == Cell::PlayerOne)
			player = Cell::PlayerTwo;
		else
			player = Cell::PlayerOne;

		moveNumber++;
	}

	return record;
}


int main() {
	const int numberOfGames = 100;
	const unsigned int seed = 42;
	std::mt19937 generator(seed);
	std::vector<gameRecord> games;

	games.reserve(numberOfGames);

	int playerOneWins = 0;
	int playerTwoWins = 0;
	int draws = 0;

	for (int gameId = 0; gameId < numberOfGames; gameId++) {
		gameRecord record = gameSim(gameId, generator);
		games.push_back(record);

		if (record.winnerAfterGame == Cell::PlayerOne)
			playerOneWins++;
		else if (record.winnerAfterGame == Cell::PlayerTwo)
			playerTwoWins++;
		else
			draws++;
	}

	std::cout << "Simulated games: " << games.size() << "\n";
	std::cout << "Player One wins: " << playerOneWins << "\n";
	std::cout << "Player Two wins: " << playerTwoWins << "\n";
	std::cout << "Draws: " << draws << "\n";

	if (!games.empty()) {
		std::cout << "First game recorded moves: ";

		for (const auto& move : games[0].moves) {
			printBoard(move.boardBeforeMove); 
	}

		std::cout << "\n";
}

	return 0;
}
