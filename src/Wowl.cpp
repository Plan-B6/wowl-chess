#include "Wowl.h"

//Hash Wowl::hashTable;

int Wowl::SEE(Board b, Evaluation& e, int square, int color) const {
	int oldsqr;
	int val = 0;
	int piece = std::get<0>(b.getSmallestAttacker(square, color));
	int target = b.mailbox[square];
	int targetval = target;

	if (piece > 0) {
		if (!(target == -9 || target == 0)) {
			oldsqr = std::get<1>(b.getSmallestAttacker(square, color));
			b.move(oldsqr, square);
			if (b.checkKing(b.getTurn() * -1, b.mailbox)) {
				b.undo();
			}
			else {
				switch (abs(target)) {
				case b.WP:
					targetval = e.P_BASE_VAL;
					break;
				case b.WN:
					targetval = e.N_BASE_VAL;
					break;
				case b.WB:
					targetval = e.B_BASE_VAL;
					break;
				case b.WR:
					targetval = e.R_BASE_VAL;
					break;
				case b.WQ:
					targetval = e.Q_BASE_VAL;
					break;
				case b.WK:
					targetval = e.K_BASE_VAL;
					break;
				}
				assert(targetval > 0);
				int netMaterial = targetval - SEE(b, e, square, -color);
				if (netMaterial > 0) {
					val = netMaterial;
				}
				else {
					val = 0;
				}
				b.undo();
			}
		}
	}
	return val;
}

void Wowl::orderMoves(Board& b, Evaluation& e, std::vector<sf::Vector2i>& lmV, int depth, int initial, U64 poskey) {
	if (hashTable.tt.find(poskey) != hashTable.tt.end()) {
		int val = hashTable.tt.at(poskey).hashBestMove;
		hashMove.x = (val - val % 100) / 100;
		hashMove.y = val % 100;
	}
	for (int i = 0; i < lmV.size(); i++) {
		//Move stored in hash table
		if (lmV[i] == hashMove) {
			lmV.insert(lmV.begin(), lmV[i]);
			lmV.erase(lmV.begin() + i + 1);
			continue;
		}
		//Good captures
		if (SEE(b, e, lmV[i].y, b.getTurn()) > 0 && std::get<1>(b.getSmallestAttacker(lmV[i].y, b.getTurn())) == lmV[i].x) {
			lmV.insert(lmV.begin(), lmV[i]);
			lmV.erase(lmV.begin() + i + 1);
			continue;
		}
		//Killer moves
		if (lmV[i].x == (killerMoves[0][depth] - killerMoves[0][depth] % 100) / 100 && lmV[i].y == killerMoves[0][depth] % 100) {
			lmV.insert(lmV.begin(), lmV[i]);
			lmV.erase(lmV.begin() + i + 1);
			continue;
		}
		if (lmV[i].x == (killerMoves[1][depth] - killerMoves[1][depth] % 100) / 100 && lmV[i].y == killerMoves[1][depth] % 100) {
			lmV.insert(lmV.begin(), lmV[i]);
			lmV.erase(lmV.begin() + i + 1);
			continue;
		}
	}
}
void Wowl::resetKillerMoves() {
	for (int i = 0; i < SEARCH_DEPTH; i++) {
		killerMoves[0][i] = 0;
		killerMoves[1][i] = 0;
	}
}

int Wowl::probeHashTable(U64 key, int depth, int alpha, int beta) {
	if (hashTable.tt.find(key) != hashTable.tt.end()) {
		if (hashTable.tt.at(key).hashDepth >= depth) {
			if (hashTable.tt.at(key).hashFlag == hashTable.HASH_EXACT) {
				return hashTable.tt.at(key).hashScore;
			}
			else if (hashTable.tt.at(key).hashFlag == hashTable.HASH_ALPHA) {
				if (hashTable.tt.at(key).hashScore <= alpha) {
					return alpha;
				}
			}
			else if (hashTable.tt.at(key).hashFlag == hashTable.HASH_BETA) {
				if (hashTable.tt.at(key).hashScore >= beta) {
					return beta;
				}
			}
		}
		return VAL_UNKWOWN;
	}
	return VAL_UNKWOWN;
}
void Wowl::recordHash(U64 key, int depth, int score, int flag) {
	hashTable.tt[key].hashDepth = depth;
	hashTable.tt[key].hashScore = score;
	hashTable.tt[key].hashFlag = flag;
	hashTable.tt[key].hashAge = 0;
}

int Wowl::qSearch(Board b, Evaluation& e, int alpha, int beta, int color) {

	int score;
	Evaluation qEval;

	int stand_pat = qEval.totalEvaluation(b, color);

	if (stand_pat >= beta) {
		return beta;
	}
	if (stand_pat > alpha) {
		alpha = stand_pat;
	}

	b.getQMoves();
	for (const auto& j : b.qMoveVec) {
		if (SEE(b, e, j.y, color) > 0) {
			qSearchNodes++;
			b.move(j.x, j.y);
			score = -qSearch(b, e, -beta, -alpha, -color);
			b.undo();
			if (score >= beta) {
				return beta;
			}
			if (score > alpha) {
				alpha = score;
			}
		}	
	}

	return alpha;
}
int Wowl::negaSearch(Board b, int depth, int initial, int color, int alpha, int beta) {

	Evaluation WowlEval;
	U64 key = hashTable.generatePosKey(b);

	//if (probeHashTable(key, depth, alpha, beta) != VAL_UNKWOWN) { return probeHashTable(key, depth, alpha, beta); }

	if (depth == 0) {
		negaNodes++;
		int qScore = qSearch(b, WowlEval, alpha, beta, color);
		recordHash(key, depth, qScore, hashTable.HASH_EXACT);
		return qScore;
	}

	int score;
	int max = -WIN_SCORE;

	b.getLegalMoves();
	int legalcount = 0;
	for (const auto& i : b.legalMoveVec) {
		if (!b.checkMoveCheck(i.x, i.y)) {
			legalcount++;
			break;
		}
	}
	if (legalcount == 0) {
		if (b.checkKing(color, b.mailbox)) {
			return -WIN_SCORE;
		}
		else {
			return 0;
		}
	}

	orderMoves(b, WowlEval, b.legalMoveVec, depth, initial, key);
	bool firstNode = true;

	for (const auto& i : b.legalMoveVec) {
		b.move(i.x, i.y);
		if (b.checkKing(b.getTurn() * -1, b.mailbox)) {
			b.undo();
			continue;
		}
		else {
			negaNodes++;
			if (firstNode) {
				firstNode = false;
				score = -negaSearch(b, depth - 1, initial, -color, -beta, -alpha);
			}
			else {
				score = -negaSearch(b, depth - 1, initial, -color, -alpha - 1, -alpha);
				if (alpha < score && score < beta) {
					int scoutScore = -negaSearch(b, depth - 1, initial, -color, -beta, -score);
					if (scoutScore > score) {
						score = scoutScore;
					}
				}
			}
		}
		b.undo();
		if (score > max) {
			max = score;

			if (depth == SEARCH_DEPTH) {
				bestMove.x = i.x;
				bestMove.y = i.y;
				std::cout << "best move : " << bestMove.x << " " << bestMove.y << " at depth " << depth << " by " << color << std::endl;
				std::cout << "best score : " << max << std::endl << std::endl;
			}

			//Store best move in hash table (replace if depth is greater)
			if (hashTable.tt.find(key) != hashTable.tt.end()) {
				if (hashTable.tt.at(key).hashDepth <= depth) {
					hashTable.tt[key].hashBestMove = i.x * 100 + i.y;
				}
			}
			else {
				hashTable.tt[key].hashBestMove = i.x * 100 + i.y;
			}
		}
		if (score > alpha) {
			alpha = score;
		}
		if (alpha >= beta) {
			recordHash(key, depth, beta, hashTable.HASH_BETA);
			//Store killer move
			killerMoves[1][depth] = killerMoves[0][depth];
			killerMoves[0][depth] = i.x * 100 + i.y;
			break;
		}
	}

	recordHash(key, depth, beta, hashTable.HASH_ALPHA);
	return max;
}
void Wowl::DLS(Board b, int depth, int color) {

	negaNodes = 0;
	qSearchNodes = 0;
	b.setEnPassantSquare();
	resetKillerMoves();

	int id_alpha = -WIN_SCORE;
	int id_beta = WIN_SCORE;
	bool research = false;

	for (int idepth = 1; idepth < SEARCH_DEPTH; idepth++) {

		estimate = negaSearch(b, idepth, idepth, color, id_alpha, id_beta);

		std::cout << id_alpha << " " << id_beta << " at depth " << idepth << std::endl;
		std::cout << estimate << " at depth " << idepth << std::endl;

		//Break search if mate is found
		if (estimate == WIN_SCORE || estimate == -WIN_SCORE) {
			id_alpha = -WIN_SCORE;
			id_beta = WIN_SCORE;
			break;
		}

		if ((estimate <= id_alpha) || (estimate >= id_beta)) {
			if (research) {
				id_alpha = -WIN_SCORE;
				id_beta = WIN_SCORE;
			}
			else {
				id_alpha = estimate - ASPIRATION_WINDOW * 2;
				id_beta = estimate + ASPIRATION_WINDOW * 2;
			}
			idepth--;
			research = true;
			continue;
		}

		id_alpha = estimate - ASPIRATION_WINDOW;
		id_beta = estimate + ASPIRATION_WINDOW;
		research = false;
	}

	negaSearch(b, depth, depth, color, -WIN_SCORE, WIN_SCORE);

	std::cout << "Nodes explored in negaSearch : " << negaNodes << std::endl;
	std::cout << "Nodes explored in qSearch : " << qSearchNodes << std::endl;
}

long Wowl::perft(Board b, int depth) {

	int nodes = 0;

	b.getQMoves();
	for (int i = 0; i < b.qMoveVec.size(); i++) {
		b.move(b.qMoveVec[i].x, b.qMoveVec[i].y);
		if (b.checkKing(b.getTurn() * -1, b.mailbox)) {
			b.undo();
			continue;
		}
		captures++;
		b.undo();
	}

	if (depth == 0) { return 1; }

	b.getLegalMoves();
	int size = b.legalMoveVec.size();

	for (int i = 0; i < size; i++) {
		b.move(b.legalMoveVec[i].x, b.legalMoveVec[i].y);
		if (b.checkKing(b.getTurn() * -1, b.mailbox)) {
			b.undo();
			continue;
		}
		nodes += perft(b, depth - 1);
		b.undo();
		if (depth == SEARCH_DEPTH) {
			std::cout << "Nodes : " << nodes << std::endl;
			std::cout << "Captures : " << captures << std::endl;
		}
	}
	return nodes;
}