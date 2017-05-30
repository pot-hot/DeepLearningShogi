﻿#define BOOST_PYTHON_STATIC_LIB
#define BOOST_NUMPY_STATIC_LIB
#include <boost/python/numpy.hpp>
#include <numeric>
#include <algorithm>

#include "init.hpp"
#include "position.hpp"
#include "search.hpp"
#include "generateMoves.hpp"

namespace py = boost::python;
namespace np = boost::python::numpy;

#define LEN(array) (sizeof(array) / sizeof(array[0]))

const int MAX_HPAWN_NUM = 8; // 歩の持ち駒の上限
const int MAX_HLANCE_NUM = 4;
const int MAX_HKNIGHT_NUM = 4;
const int MAX_HSILVER_NUM = 4;
const int MAX_HGOLD_NUM = 4;
const int MAX_HBISHOP_NUM = 2;
const int MAX_HROOK_NUM = 2;

const u32 MAX_PIECES_IN_HAND[] = {
	MAX_HPAWN_NUM, // PAWN
	MAX_HLANCE_NUM, // LANCE
	MAX_HKNIGHT_NUM, // KNIGHT
	MAX_HSILVER_NUM, // SILVER
	MAX_HGOLD_NUM, // GOLD
	MAX_HBISHOP_NUM, // BISHOP
	MAX_HROOK_NUM, // ROOK
};
const u32 MAX_PIECES_IN_HAND_SUM = MAX_HPAWN_NUM + MAX_HLANCE_NUM + MAX_HKNIGHT_NUM + MAX_HSILVER_NUM + MAX_HGOLD_NUM + MAX_HBISHOP_NUM + MAX_HROOK_NUM;
const u32 MAX_FEATURES2_HAND_NUM = (int)ColorNum * MAX_PIECES_IN_HAND_SUM;
const u32 MAX_FEATURES2_NUM = MAX_FEATURES2_HAND_NUM + 1/*王手*/;

// 移動の定数
enum MOVE_DIRECTION {
	UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN, DOWN_LEFT, DOWN_RIGHT,
	UP_PROMOTE, UP_LEFT_PROMOTE, UP_RIGHT_PROMOTE, LEFT_PROMOTE, RIGHT_PROMOTE, DOWN_PROMOTE, DOWN_LEFT_PROMOTE, DOWN_RIGHT_PROMOTE
};

const MOVE_DIRECTION MOVE_DIRECTION_PROMOTED[] = {
	UP_PROMOTE, UP_LEFT_PROMOTE, UP_RIGHT_PROMOTE, LEFT_PROMOTE, RIGHT_PROMOTE, DOWN_PROMOTE, DOWN_LEFT_PROMOTE, DOWN_RIGHT_PROMOTE
};

const MOVE_DIRECTION PAWN_MOVE_DIRECTION[] = { UP, UP_PROMOTE };
const MOVE_DIRECTION LANCE_MOVE_DIRECTION[] = { UP, UP_PROMOTE };
const MOVE_DIRECTION KNIGHT_MOVE_DIRECTION[] = {
	UP_LEFT, UP_RIGHT,
	UP_LEFT_PROMOTE, UP_RIGHT_PROMOTE };
const MOVE_DIRECTION SILVER_MOVE_DIRECTION[] = {
	UP, UP_LEFT, UP_RIGHT, DOWN_LEFT, DOWN_RIGHT,
	UP_PROMOTE, UP_LEFT_PROMOTE, UP_RIGHT_PROMOTE, DOWN_LEFT_PROMOTE, DOWN_RIGHT_PROMOTE };
const MOVE_DIRECTION BISHOP_MOVE_DIRECTION[] = {
	UP_LEFT, UP_RIGHT, DOWN_LEFT, DOWN_RIGHT,
	UP_LEFT_PROMOTE, UP_RIGHT_PROMOTE, DOWN_LEFT_PROMOTE, DOWN_RIGHT_PROMOTE };
const MOVE_DIRECTION ROOK_MOVE_DIRECTION[] = {
	UP, LEFT, RIGHT, DOWN,
	UP_PROMOTE, LEFT_PROMOTE, RIGHT_PROMOTE, DOWN_PROMOTE };
const MOVE_DIRECTION GOLD_MOVE_DIRECTION[] = {
	UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN,
	UP_PROMOTE, UP_LEFT_PROMOTE, UP_RIGHT_PROMOTE, LEFT_PROMOTE, RIGHT_PROMOTE, DOWN };
const MOVE_DIRECTION KING_MOVE_DIRECTION[] = { UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN, DOWN_LEFT, DOWN_RIGHT };
const MOVE_DIRECTION PROM_PAWN_MOVE_DIRECTION[] = { UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN };
const MOVE_DIRECTION PROM_LANCE_MOVE_DIRECTION[] = { UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN };
const MOVE_DIRECTION PROM_KNIGHT_MOVE_DIRECTION[] = { UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN };
const MOVE_DIRECTION PROM_SILVER_MOVE_DIRECTION[] = { UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN };
const MOVE_DIRECTION PROM_BISHOP_MOVE_DIRECTION[] = { UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN, DOWN_LEFT, DOWN_RIGHT };
const MOVE_DIRECTION PROM_ROOK_MOVE_DIRECTION[] = { UP, UP_LEFT, UP_RIGHT, LEFT, RIGHT, DOWN, DOWN_LEFT, DOWN_RIGHT };

const int PIECE_MOVE_DIRECTION_INDEX[][16] = {
	{ 0, -1, -1, -1, -1, -1, -1, -1, 1, -1, -1, -1, -1, -1, -1, -1 }, // PAWN
	{ 0, -1, -1, -1, -1, -1, -1, -1, 1, -1, -1, -1, -1, -1, -1, -1 }, // LANCE
	{ 0, 1, 2, 3, 4, -1, -1, -1, -1, 5, 6, 7, 8, -1, -1, -1 }, // KNIGHT
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, -1, 13, 14 }, // SILVER
	{ 0, 1, 2, 3, 4, 5, 6, 7, -1, 8, 9, 10, 11, -1, 12, 13 }, // BISHOP
	{ 0, -1, -1, 1, 2, 3, -1, -1, 4, -1, -1, 5, 6, 7, -1, -1 }, // ROOK
	{ 0, 1, 2, 3, 4, 5, -1, -1, 6, 7, 8, 9, 10, -1, -1, -1 }, // GOLD
	{ 0, 1, 2, 3, 4, 5, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1 }, // KING
	{ 0, 1, 2, 3, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // PROM_PAWN
	{ 0, 1, 2, 3, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // PROM_LANCE
	{ 0, 1, 2, 3, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // PROM_KNIGHT
	{ 0, 1, 2, 3, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, // PROM_SILVER
	{ 0, 1, 2, 3, 4, 5, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1 }, // PROM_BISHOP
	{ 0, 1, 2, 3, 4, 5, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1 }, // PROM_ROOK
};

// classification label
const int PAWN_MOVE_DIRECTION_LABEL = 0;
const int LANCE_MOVE_DIRECTION_LABEL = LEN(PAWN_MOVE_DIRECTION);
const int KNIGHT_MOVE_DIRECTION_LABEL = LANCE_MOVE_DIRECTION_LABEL + LEN(LANCE_MOVE_DIRECTION);
const int SILVER_MOVE_DIRECTION_LABEL = KNIGHT_MOVE_DIRECTION_LABEL + LEN(KNIGHT_MOVE_DIRECTION);
const int BISHOP_MOVE_DIRECTION_LABEL = SILVER_MOVE_DIRECTION_LABEL + LEN(GOLD_MOVE_DIRECTION);
const int ROOK_MOVE_DIRECTION_LABEL = BISHOP_MOVE_DIRECTION_LABEL + LEN(BISHOP_MOVE_DIRECTION);
const int GOLD_MOVE_DIRECTION_LABEL = ROOK_MOVE_DIRECTION_LABEL + LEN(SILVER_MOVE_DIRECTION);
const int KING_MOVE_DIRECTION_LABEL = GOLD_MOVE_DIRECTION_LABEL + LEN(ROOK_MOVE_DIRECTION);
const int PROM_PAWN_MOVE_DIRECTION_LABEL = KING_MOVE_DIRECTION_LABEL + LEN(KING_MOVE_DIRECTION);
const int PROM_LANCE_MOVE_DIRECTION_LABEL = PROM_PAWN_MOVE_DIRECTION_LABEL + LEN(PROM_PAWN_MOVE_DIRECTION);
const int PROM_KNIGHT_MOVE_DIRECTION_LABEL = PROM_LANCE_MOVE_DIRECTION_LABEL + LEN(PROM_LANCE_MOVE_DIRECTION);
const int PROM_SILVER_MOVE_DIRECTION_LABEL = PROM_KNIGHT_MOVE_DIRECTION_LABEL + LEN(PROM_KNIGHT_MOVE_DIRECTION);
const int PROM_BISHOP_MOVE_DIRECTION_LABEL = PROM_SILVER_MOVE_DIRECTION_LABEL + LEN(PROM_SILVER_MOVE_DIRECTION);
const int PROM_ROOK_MOVE_DIRECTION_LABEL = PROM_BISHOP_MOVE_DIRECTION_LABEL + LEN(PROM_BISHOP_MOVE_DIRECTION);
const int MOVE_DIRECTION_LABEL_NUM = PROM_ROOK_MOVE_DIRECTION_LABEL + LEN(PROM_ROOK_MOVE_DIRECTION);

const int PIECE_MOVE_DIRECTION_LABEL[] = {
	0,
	PAWN_MOVE_DIRECTION_LABEL, LANCE_MOVE_DIRECTION_LABEL, KNIGHT_MOVE_DIRECTION_LABEL, SILVER_MOVE_DIRECTION_LABEL,
	BISHOP_MOVE_DIRECTION_LABEL, ROOK_MOVE_DIRECTION_LABEL,
	GOLD_MOVE_DIRECTION_LABEL,
	KING_MOVE_DIRECTION_LABEL,
	PROM_PAWN_MOVE_DIRECTION_LABEL, PROM_LANCE_MOVE_DIRECTION_LABEL, PROM_KNIGHT_MOVE_DIRECTION_LABEL, PROM_SILVER_MOVE_DIRECTION_LABEL,
	PROM_BISHOP_MOVE_DIRECTION_LABEL, PROM_ROOK_MOVE_DIRECTION_LABEL
};

// make input features
inline void make_input_features(const Position& position, float(*features1)[ColorNum][PieceTypeNum - 1][SquareNum], float(*features2)[MAX_FEATURES2_NUM][SquareNum]) {
	float(*features2_hand)[ColorNum][MAX_PIECES_IN_HAND_SUM][SquareNum] = reinterpret_cast<float(*)[ColorNum][MAX_PIECES_IN_HAND_SUM][SquareNum]>(features2);
	for (Color c = Black; c < ColorNum; ++c) {
		// 白の場合、色を反転
		Color c2 = c;
		if (position.turn() == White) {
			c2 = oppositeColor(c);
		}

		for (PieceType pt = Pawn; pt < PieceTypeNum; ++pt) {
			Bitboard bb = position.bbOf(pt, c);
			for (Square sq = SQ11; sq < SquareNum; ++sq) {
				// 白の場合、盤面を180度回転
				Square sq2 = sq;
				if (position.turn() == White) {
					sq2 = SQ99 - sq;
				}

				if (bb.isSet(sq)) {
					(*features1)[c2][pt - 1][sq2] = 1.0f;
				}
			}
		}
		// hand
		Hand hand = position.hand(c);
		int p = 0;
		for (HandPiece hp = HPawn; hp < HandPieceNum; ++hp) {
			u32 num = hand.numOf(hp);
			if (num >= MAX_PIECES_IN_HAND[hp]) {
				num = MAX_PIECES_IN_HAND[hp];
			}
			std::fill_n((*features2_hand)[c2][p], (int)SquareNum * num, 1.0f);
			p += MAX_PIECES_IN_HAND[hp];
		}
	}

	// is check
	if (position.inCheck()) {
		std::fill_n((*features2)[MAX_FEATURES2_HAND_NUM], SquareNum, 1.0f);
	}
}

// make result
inline float make_result(const GameResult gameResult, const Position& position) {
	if (gameResult == Draw) {
		return 0.0f;
	}
	else {
		if (position.turn() == Black && gameResult == WhiteWin ||
			position.turn() == White && gameResult == BlackWin) {
			return -1.0f;
		}
		else {
			return 1.0f;
		}
	}
}

// make move
inline int make_move_label(const u16 move16, const Position& position) {
	// see: move.hpp : 30
	// xxxxxxxx x1111111  移動先
	// xx111111 1xxxxxxx  移動元。駒打ちの際には、PieceType + SquareNum - 1
	// x1xxxxxx xxxxxxxx  1 なら成り
	u16 to_sq = move16 & 0b1111111;
	u16 from_sq = (move16 >> 7) & 0b1111111;

	// move direction
	int move_direction_label;
	if (from_sq < SquareNum) {
		// 駒の種類
		PieceType move_piece = pieceToPieceType(position.piece((Square)from_sq));

		// 白の場合、盤面を180度回転
		if (position.turn() == White) {
			to_sq = (u16)SQ99 - to_sq;
			from_sq = (u16)SQ99 - from_sq;
		}

		const div_t to_d = div(to_sq, 9);
		const int to_x = to_d.quot;
		const int to_y = to_d.rem;
		const div_t from_d = div(from_sq, 9);
		const int from_x = from_d.quot;
		const int from_y = from_d.rem;
		const int dir_x = from_x - to_x;
		const int dir_y = to_y - from_y;

		MOVE_DIRECTION move_direction;
		if (dir_y < 0 && dir_x == 0) {
			move_direction = UP;
		}
		else if (dir_y < 0 && dir_x < 0) {
			move_direction = UP_LEFT;
		}
		else if (dir_y < 0 && dir_x > 0) {
			move_direction = UP_RIGHT;
		}
		else if (dir_y == 0 && dir_x < 0) {
			move_direction = LEFT;
		}
		else if (dir_y == 0 && dir_x > 0) {
			move_direction = RIGHT;
		}
		else if (dir_y > 0 && dir_x == 0) {
			move_direction = DOWN;
		}
		else if (dir_y > 0 && dir_x < 0) {
			move_direction = DOWN_LEFT;
		}
		else if (dir_y > 0 && dir_x > 0) {
			move_direction = DOWN_RIGHT;
		}

		// promote
		if ((move16 & 0b100000000000000) > 0) {
			move_direction = MOVE_DIRECTION_PROMOTED[move_direction];
		}
		move_direction_label = PIECE_MOVE_DIRECTION_LABEL[move_piece] + PIECE_MOVE_DIRECTION_INDEX[move_piece][move_direction];
	}
	// 持ち駒の場合
	else {
		// 白の場合、盤面を180度回転
		if (position.turn() == White) {
			to_sq = (u16)SQ99 - to_sq;
		}
		const int hand_piece = from_sq - (int)SquareNum;
		move_direction_label = MOVE_DIRECTION_LABEL_NUM + hand_piece;
	}

	return 9 * 9 * move_direction_label + to_sq;
}

/*
	HuffmanCodedPosAndEvalの配列からpolicy networkの入力ミニバッチに変換する。
	ndhcpe : Python側で以下の構造体を定義して、その配列を入力する。
		HuffmanCodedPosAndEval = np.dtype([
			('hcp', np.uint8, 32),
			('eval', np.int16),
			('bestMove16', np.uint16),
			('gameResult', np.uint8),
			('dummy', np.uint8),
		])
	ndfeatures1, ndfeatures2, ndresult : 変換結果を受け取る。Python側でnp.emptyで事前に領域を確保する。
*/
void hcpe_decode_with_result(np::ndarray ndhcpe, np::ndarray ndfeatures1, np::ndarray ndfeatures2, np::ndarray ndresult) {
	const int len = (int)ndhcpe.shape(0);
	HuffmanCodedPosAndEval *hcpe = reinterpret_cast<HuffmanCodedPosAndEval *>(ndhcpe.get_data());
	float (*features1)[ColorNum][PieceTypeNum-1][SquareNum] = reinterpret_cast<float(*)[ColorNum][PieceTypeNum-1][SquareNum]>(ndfeatures1.get_data());
	float (*features2)[MAX_FEATURES2_NUM][SquareNum] = reinterpret_cast<float(*)[MAX_FEATURES2_NUM][SquareNum]>(ndfeatures2.get_data());
	float *result = reinterpret_cast<float *>(ndresult.get_data());

	// set all zero
	std::fill_n((float*)features1, (int)ColorNum * (PieceTypeNum-1) * (int)SquareNum * len, 0.0f);
	std::fill_n((float*)features2, MAX_FEATURES2_NUM * (int)SquareNum * len, 0.0f);

	Position position;
	for (int i = 0; i < len; i++, hcpe++, features1++, features2++, result++) {
		position.set(hcpe->hcp, nullptr);

		// input features
		make_input_features(position, features1, features2);

		// game result
		*result = make_result(hcpe->gameResult, position);
	}
}

void hcpe_decode_with_move(np::ndarray ndhcpe, np::ndarray ndfeatures1, np::ndarray ndfeatures2, np::ndarray ndmove) {
	const int len = (int)ndhcpe.shape(0);
	HuffmanCodedPosAndEval *hcpe = reinterpret_cast<HuffmanCodedPosAndEval *>(ndhcpe.get_data());
	float(*features1)[ColorNum][PieceTypeNum - 1][SquareNum] = reinterpret_cast<float(*)[ColorNum][PieceTypeNum - 1][SquareNum]>(ndfeatures1.get_data());
	float(*features2)[MAX_FEATURES2_NUM][SquareNum] = reinterpret_cast<float(*)[MAX_FEATURES2_NUM][SquareNum]>(ndfeatures2.get_data());
	int *move = reinterpret_cast<int *>(ndmove.get_data());

	// set all zero
	std::fill_n((float*)features1, (int)ColorNum * (PieceTypeNum - 1) * (int)SquareNum * len, 0.0f);
	std::fill_n((float*)features2, MAX_FEATURES2_NUM * (int)SquareNum * len, 0.0f);

	Position position;
	for (int i = 0; i < len; i++, hcpe++, features1++, features2++, move++) {
		position.set(hcpe->hcp, nullptr);

		// input features
		make_input_features(position, features1, features2);

		// move
		*move = make_move_label(hcpe->bestMove16, position);
	}
}

void hcpe_decode_with_value(np::ndarray ndhcpe, np::ndarray ndfeatures1, np::ndarray ndfeatures2, np::ndarray ndvalue, np::ndarray ndmove, np::ndarray ndresult) {
	const int len = (int)ndhcpe.shape(0);
	HuffmanCodedPosAndEval *hcpe = reinterpret_cast<HuffmanCodedPosAndEval *>(ndhcpe.get_data());
	float(*features1)[ColorNum][PieceTypeNum - 1][SquareNum] = reinterpret_cast<float(*)[ColorNum][PieceTypeNum - 1][SquareNum]>(ndfeatures1.get_data());
	float(*features2)[MAX_FEATURES2_NUM][SquareNum] = reinterpret_cast<float(*)[MAX_FEATURES2_NUM][SquareNum]>(ndfeatures2.get_data());
	float *value = reinterpret_cast<float *>(ndvalue.get_data());
	int *move = reinterpret_cast<int *>(ndmove.get_data());
	float *result = reinterpret_cast<float *>(ndresult.get_data());

	// set all zero
	std::fill_n((float*)features1, (int)ColorNum * (PieceTypeNum - 1) * (int)SquareNum * len, 0.0f);
	std::fill_n((float*)features2, MAX_FEATURES2_NUM * (int)SquareNum * len, 0.0f);

	Position position;
	for (int i = 0; i < len; i++, hcpe++, features1++, features2++, value++, move++, result++) {
		position.set(hcpe->hcp, nullptr);

		// input features
		make_input_features(position, features1, features2);

		// eval
		*value = tanh((float)hcpe->eval * 0.00067492f);

		// move
		*move = make_move_label(hcpe->bestMove16, position);

		// game result
		*result = make_result(hcpe->gameResult, position);
	}
}

// Boltzmann distribution
// see: Reinforcement Learning : An Introduction 2.3.SOFTMAX ACTION SELECTION
void softmax_tempature(std::vector<float> &log_probabilities) {
	//const float tempature = 0.67;
	const float tempature = 0.3;
	const float beta = 1.0f / tempature;
	// apply beta exponent to probabilities(in log space)
	for (float& x : log_probabilities) {
		x = expf(x * beta);
	}
}

class Engine {
private:
	Searcher *m_searcher;
	Position *m_position;
	static bool evalTableIsRead;
public:
	void init(const char* eval_dir) {
		m_searcher = new Searcher();
		m_position = new Position(m_searcher);

		if (!evalTableIsRead) {
			std::unique_ptr<Evaluator>(new Evaluator)->init(eval_dir, true);
		}
		m_searcher->init();
		const std::string options[] = {
			"name Threads value 1",
			"name MultiPV value 1",
			"name USI_Hash value 256",
			"name OwnBook value false",
			"name Max_Random_Score_Diff value 0" };
		for (auto& str : options) {
			std::istringstream is(str);
			m_searcher->setOption(is);
		}
	}

	void position(const char* cmd) {
		std::istringstream ssCmd(cmd);
		setPosition(*m_position, ssCmd);
		//g_position->print();
	}

	py::list go() {
		m_position->searcher()->alpha = -ScoreMaxEvaluate;
		m_position->searcher()->beta = ScoreMaxEvaluate;

		// go
		LimitsType limits;
		limits.depth = static_cast<Depth>(6);
		m_position->searcher()->threads.startThinking(*m_position, limits, m_position->searcher()->states);
		m_position->searcher()->threads.main()->waitForSearchFinished();

		const Score score = m_position->searcher()->threads.main()->rootMoves[0].score;
		const Move bestMove = m_position->searcher()->threads.main()->rootMoves[0].pv[0];

		std::cout << "info score cp " << score << " pv " << bestMove.toUSI() << std::endl;

		py::list ret;
		ret.append(bestMove.toUSI());
		ret.append((int)score);
		return ret;
	}

	int make_input_features(np::ndarray ndfeatures1, np::ndarray ndfeatures2) {
		float(*features1)[ColorNum][PieceTypeNum - 1][SquareNum] = reinterpret_cast<float(*)[ColorNum][PieceTypeNum - 1][SquareNum]>(ndfeatures1.get_data());
		float(*features2)[MAX_FEATURES2_NUM][SquareNum] = reinterpret_cast<float(*)[MAX_FEATURES2_NUM][SquareNum]>(ndfeatures2.get_data());

		// set all zero
		std::fill_n((float*)features1, (int)ColorNum * (PieceTypeNum - 1) * (int)SquareNum, 0.0f);
		std::fill_n((float*)features2, MAX_FEATURES2_NUM * (int)SquareNum, 0.0f);

		// input features
		::make_input_features(*m_position, features1, features2);

		return (int)m_position->turn();
	}

	const std::string select_move(np::ndarray ndlogits) {
		float *logits = reinterpret_cast<float*>(ndlogits.get_data());

		// 合法手一覧
		std::vector<Move> legal_moves;
		std::vector<float> legal_move_probabilities;
		for (MoveList<Legal> ml(*m_position); !ml.end(); ++ml) {
			const Move move = ml.move();

			const int move_label = make_move_label((u16)move.proFromAndTo(), *m_position);

			legal_moves.emplace_back(move);
			legal_move_probabilities.emplace_back(logits[move_label]);
		}

		/*for (int i = 0; i < legal_move_probabilities.size(); i++) {
		std::cout << "info string i:" << i << " move:" << legal_moves[i].toUSI() << " p:" << legal_move_probabilities[i] << std::endl;
		}*/

		// Boltzmann distribution
		softmax_tempature(legal_move_probabilities);

		for (int i = 0; i < legal_move_probabilities.size(); i++) {
			std::cout << "info string i:" << i << " move:" << legal_moves[i].toUSI() << " temp_p:" << legal_move_probabilities[i] << std::endl;
		}

		// 確率に応じて手を選択
		std::discrete_distribution<int> distribution(legal_move_probabilities.begin(), legal_move_probabilities.end());
		int move_idx = distribution(g_randomTimeSeed);

		// greedy
		//int move_idx = std::distance(legal_move_probabilities.begin(), std::max_element(legal_move_probabilities.begin(), legal_move_probabilities.end()));

		Move move = legal_moves[move_idx];

		// ラベルからusiに変換
		return move.toUSI();
	}
};
bool Engine::evalTableIsRead = false;

BOOST_PYTHON_MODULE(cppshogi) {
	Py_Initialize();
	np::initialize();

	initTable();
	Position::initZobrist();
	HuffmanCodedPos::init();

	py::def("hcpe_decode_with_result", hcpe_decode_with_result);
	py::def("hcpe_decode_with_move", hcpe_decode_with_move);
	py::def("hcpe_decode_with_value", hcpe_decode_with_value);

	py::class_<Engine>("Engine")
		.def("init", &Engine::init)
		.def("position", &Engine::position)
		.def("go", &Engine::go)
		.def("make_input_features", &Engine::make_input_features)
		.def("select_move", &Engine::select_move);
}