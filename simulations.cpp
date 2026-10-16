#include <vector>
#include <random>
#include <iostream>
#include <filesystem>
#include <string>

#include "connect_four.h"
#include "nnet/nn/checkpoint.h"

const std::string checkpointFolder = "build/connect4/";

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

		record.moves.push_back({moveNumber, player, boardBeforeMove, chosenCol});

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


int randomMove(const Board& board, std::mt19937& generator) {
	std::vector<int> legalColumns;
	for (int col = 0; col < 7; col++) {
		if (!columnFull(board, col))
			legalColumns.push_back(col);
	}
	std::uniform_int_distribution<int> distribution(0, static_cast<int>(legalColumns.size()) - 1);
	return legalColumns[distribution(generator)];
}

// win if possible, block if the opponent could win next, otherwise random
int ruleBasedMove(const Board& board, Cell player, std::mt19937& generator) {
	for (const Cell who : {player, other(player)}) {
		for (int col = 0; col < 7; col++) {
			if (columnFull(board, col)) continue;
			Board next = board;
			const int row = dropPiece(next, col, who);
			if (getWinner(next, row, col) == who) return col;
		}
	}
	return randomMove(board, generator);
}

// the model plays itself, with some random moves so the games vary
gameRecord selfPlayGame(nnet::Sequential& model, double randomMoveChance, std::mt19937& generator) {
	gameRecord record{0, {}, Cell::Empty};
	Board board;
	for (auto& row : board) row.fill(Cell::Empty);
	std::uniform_real_distribution<double> coin(0.0, 1.0);
	Cell player = Cell::PlayerOne;
	for (int moveNumber = 0; !boardFull(board); moveNumber++) {
		const int col = coin(generator) < randomMoveChance ? randomMove(board, generator)
		                                                   : chooseMove(model, board, player);
		record.moves.push_back({moveNumber, player, board, col});
		const int row = dropPiece(board, col, player);
		if (getWinner(board, row, col) == player) {
			record.winnerAfterGame = player;
			break;
		}
		player = other(player);
	}
	return record;
}

// wins for `model` out of 400 games against `opponent`, or against the
// rule-based player when opponent is null; sides alternate, 10% random moves
int winsOutOf400(nnet::Sequential& model, nnet::Sequential* opponent) {
	std::mt19937 generator(321);
	std::uniform_real_distribution<double> coin(0.0, 1.0);
	int wins = 0;
	for (int game = 0; game < 400; game++) {
		const Cell modelPiece = game % 2 == 0 ? Cell::PlayerOne : Cell::PlayerTwo;
		Board board;
		for (auto& row : board) row.fill(Cell::Empty);
		Cell player = Cell::PlayerOne;
		while (!boardFull(board)) {
			int col;
			if (coin(generator) < 0.1) col = randomMove(board, generator);
			else if (player == modelPiece) col = chooseMove(model, board, player);
			else if (opponent) col = chooseMove(*opponent, board, player);
			else col = ruleBasedMove(board, player, generator);
			const int row = dropPiece(board, col, player);
			if (getWinner(board, row, col) == player) {
				if (player == modelPiece) wins++;
				break;
			}
			player = other(player);
		}
	}
	return wins;
}


int main(int argc, char* argv[]) {
	const int numberOfGames = 50;
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

	// generation 0 learns from the random games above
	std::filesystem::create_directories(checkpointFolder);
	const std::string bestFile = checkpointFolder + "best.ckpt";
	auto best = buildValueNetwork(generator);
	trainAI(*best, games, 5, 0.001);
	nnet::saveParameters(*best, bestFile);
	nnet::saveParameters(*best, checkpointFolder + "gen_0.ckpt");
	std::cout << "\ngeneration 0: " << winsOutOf400(*best, nullptr) << "/400 against the rule-based player\n";

	// each round: self-play, train a candidate on the last few rounds of games,
	// and keep it only if it beats the current best
	// connect4_simulations 0 trains on the random games only
	const int rounds = argc > 1 ? std::stoi(argv[1]) : 12;
	std::vector<std::vector<gameRecord>> recentGames;
	int generation = 0;
	for (int round = 1; round <= rounds; round++) {
		std::vector<gameRecord> selfPlay;
		for (int game = 0; game < 3000; game++)
			selfPlay.push_back(selfPlayGame(*best, 0.2, generator));
		recentGames.push_back(std::move(selfPlay));
		if (recentGames.size() > 4)
			recentGames.erase(recentGames.begin());

		std::vector<gameRecord> trainingGames;
		for (const auto& roundGames : recentGames)
			trainingGames.insert(trainingGames.end(), roundGames.begin(), roundGames.end());

		auto candidate = buildValueNetwork(generator);
		nnet::loadParameters(*candidate, bestFile);
		trainAI(*candidate, trainingGames, 2, 0.0005);

		const int candidateWins = winsOutOf400(*candidate, best.get());
		const bool accepted = candidateWins >= 220;
		if (accepted) {
			generation++;
			nnet::saveParameters(*candidate, bestFile);
			nnet::saveParameters(*candidate, checkpointFolder + "gen_" + std::to_string(generation) + ".ckpt");
			nnet::loadParameters(*best, bestFile);
		}
		std::cout << "round " << round << ": candidate won " << candidateWins << "/400 against the best, "
		          << (accepted ? "accepted" : "rejected") << " | generation " << generation << ": "
		          << winsOutOf400(*best, nullptr) << "/400 against the rule-based player\n";
	}

	return 0;
}
