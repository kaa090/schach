// Возможности:
// + выгрузка FEN
// + загрузка FEN
// + редактор
// Доработать:
// - выбор фигуры превращения для пешки - сейчас только ферзь
// - подсветка шаха и мата
// - подсветка последнего хода
// - проверка короля на шах
// - проверка рокировки
// - stockfish для linux

// Управление:
// Escape - выход
// BackSpace - отмена хода
// Space - сделать ход движком
// F - напечатать FEN
// U - напечатать UCI
// P - напечатать myPGN
// G - напечатать рекомендуемый движком ход
// B - напечатать в текстовом виде ситуацию на доске 
// E - включить/выключить движок
// M - режим редактирования позиции

#include <iostream>
#include <SFML/Graphics.hpp>

#include <windows.h>
#include <stdio.h>
#include <string>
#include <sstream>

using namespace sf;

const int BLACKSIDE = 0;
const int ENGINE_DEPTH = 15; // [1, 20] белыми обыгрываю [1, 7]
const bool PLAY_WITH_ENGINE = 1;
const int CHESS_SIZE = 0; // 0 - small, 1 - big

//CHESS_SIZE = 0:
const int SQUARE_SIZE_0 = 50;
const char* PATH_BOARD_0 = "rsc/small_board.png";
const char* PATH_PIECES_0 = "rsc/small_pieces2.png";

//CHESS_SIZE = 1:
const int SQUARE_SIZE_1 = 100;
const char* PATH_BOARD_1 = "rsc/big_board.png";
const char* PATH_PIECES_1 = "rsc/big_pieces2.png";

const char* PATH_ENGINE = "D:\\prog\\cpp\\schach\\stockfish\\stockfish.exe";

const int K = 1;
const int Q = 2;
const int B = 3;
const int N = 4;
const int R = 5;
const int P = 6;
const int ROW_WHITE = 0;
const int ROW_BLACK = 1;
const int MOVE_SIDE_WHITE = 1;
const int MOVE_SIDE_BLACK = -1;

class Chess
{
	int square_size;
	RenderWindow window;
	Sprite s_board, s_piece;
	Texture t_board, t_pieces;

	int board[8][8] = {
		{-R,-N,-B,-Q,-K,-B,-N,-R },
		{-P,-P,-P,-P,-P,-P,-P,-P },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ P, P, P, P, P, P, P, P },
		{ R, N, B, Q, K, B, N, R }
	};

	String UCI;
	std::string FEN;
	std::string myPGN;

	int counter_move = 1;
	int counter_move50 = 0;
	int counter_halfmove = 0;
	int piece_selected = 0;
	int move_side = 1;
	int castle_K, castle_Q, castle_k, castle_q;
	Vector2i enpassant;
	bool game_over = 0;
	bool play_with_engine;
	bool modify_board = 0;

	PROCESS_INFORMATION proc_info;
	HANDLE pipin_w, pipin_r, pipout_w, pipout_r;

public:

	Chess()
	{
		if (CHESS_SIZE == 0)
		{
			square_size = SQUARE_SIZE_0;
			t_board.loadFromFile(PATH_BOARD_0);
			t_pieces.loadFromFile(PATH_PIECES_0);
		}
		else if (CHESS_SIZE == 1)
		{
			square_size = SQUARE_SIZE_1;
			t_board.loadFromFile(PATH_BOARD_1);
			t_pieces.loadFromFile(PATH_PIECES_1);
		}

		s_board.setTexture(t_board);
		s_piece.setTexture(t_pieces);
		window.create(VideoMode(8*square_size, 8*square_size), "kaaChess");
		window.setFramerateLimit(60);

		counter_halfmove = 0;
		move_side = MOVE_SIDE_WHITE;
		castle_K = 1;
		castle_Q = 1;
		castle_k = 1;
		castle_q = 1;
		game_over = false;
		enpassant.x = 0;
		enpassant.y = 0;

		proc_info = {0};
		connect_engine(PATH_ENGINE);

		play_with_engine = PLAY_WITH_ENGINE;
	}

	void clear_board()
	{
		for (int i = 0; i < 8; ++i)
			for (int j = 0; j < 8; ++j)
				board[i][j] = 0;
	}

	std::string get_FEN()
	{
		std::string _FEN;
		int empty_cells;

		for (int i = 0; i < 8; i++)
		{
			empty_cells = 0;
			for (int j = 0; j < 8; j++)
			{
				if (board[i][j] == 0)
					empty_cells++;
				else
				{
					if (empty_cells)
					{
						_FEN = _FEN + std::to_string(empty_cells);
						empty_cells = 0;
					}
					if (board[i][j] == K)
						_FEN = _FEN + "K";
					else if (board[i][j] == -K)
						_FEN = _FEN + "k";
					else if (board[i][j] == Q)
						_FEN = _FEN + "Q";
					else if (board[i][j] == -Q)
						_FEN = _FEN + "q";
					else if (board[i][j] == B)
						_FEN = _FEN + "B";
					else if (board[i][j] == -B)
						_FEN = _FEN + "b";
					else if (board[i][j] == N)
						_FEN = _FEN + "N";
					else if (board[i][j] == -N)
						_FEN = _FEN + "n";
					else if (board[i][j] == R)
						_FEN = _FEN + "R";
					else if (board[i][j] == -R)
						_FEN = _FEN + "r";
					else if (board[i][j] == P)
						_FEN = _FEN + "P";
					else if (board[i][j] == -P)
						_FEN = _FEN + "p";
				}
			}
			if (empty_cells)
				_FEN = _FEN + std::to_string(empty_cells);

			_FEN = _FEN + "/";
		}
		_FEN.pop_back();

		_FEN = _FEN + " ";
		if (move_side == MOVE_SIDE_WHITE)
			_FEN = _FEN + "w";
		else if(move_side == MOVE_SIDE_BLACK)
			_FEN = _FEN + "b";

		_FEN = _FEN + " ";
		if (castle_K)
			_FEN = _FEN + "K";
		if (castle_Q)
			_FEN = _FEN + "Q";
		if (castle_k)
			_FEN = _FEN + "k";
		if (castle_q)
			_FEN = _FEN + "q";
		if (castle_K == 0 && castle_Q == 0 && castle_k == 0 && castle_q == 0)
			_FEN = _FEN + "-";

		_FEN = _FEN + " ";
		if (enpassant.x != 0 || enpassant.y != 0)
			_FEN = _FEN + get_chess_coords_by_xy(enpassant).toAnsiString();
		else
			_FEN = _FEN + "-";

		_FEN = _FEN + " " + std::to_string(counter_move50);
		_FEN = _FEN + " " + std::to_string(counter_move);

		return _FEN;
	}

	void set_FEN(std::string _FEN)
	{
		int i = 0, j = 0;
		int piece;
		int delta;

		std::string FEN_figures, FEN_active_side, FEN_castling, FEN_enpassant, FEN_move50, FEN_fullmove;
		std::string delimiter = " ";
		std::size_t found;

		std::size_t last = 0;
		std::size_t next = 0;

		clear_board();
		FEN = _FEN;


		for (int i = 0; i < 5; i++)
		{
			if ((next = FEN.find(delimiter, last)) != std::string::npos)
			{
				if (i == 0)
					FEN_figures = FEN.substr(last, next-last);
				else if (i == 1)
					FEN_active_side = FEN.substr(last, next-last);
				else if (i == 2)
					FEN_castling = FEN.substr(last, next-last);
				else if (i == 3)
					FEN_enpassant = FEN.substr(last, next-last);
				else if (i == 4)
					FEN_move50 = FEN.substr(last, next-last);

				last = next + 1;
			}
		}
		FEN_fullmove = FEN.substr(last);

		move_side = MOVE_SIDE_WHITE;
		if (FEN_active_side == "b")
			move_side = MOVE_SIDE_BLACK;

		castle_K = 0;
		castle_Q = 0;
		castle_k = 0;
		castle_q = 0;

		found = FEN_castling.find("K");
		if (found != std::string::npos)
			castle_K = 1;
		found = FEN_castling.find("Q");
		if (found != std::string::npos)
			castle_Q = 1;
		found = FEN_castling.find("k");
		if (found != std::string::npos)
			castle_k = 1;
		found = FEN_castling.find("q");
		if (found != std::string::npos)
			castle_q = 1;

		if (FEN_enpassant == "-")
		{
			enpassant.x = 0;
			enpassant.y = 0;
		}
		else
		{
			std::cout << "enpassant setted" << std::endl;
			enpassant = get_xy_by_chess_coords(FEN_enpassant);
		}

		counter_move50 = std::stoi(FEN_move50);
		counter_move = std::stoi(FEN_fullmove);
		counter_halfmove = 2 * (counter_move - 1);
		if (FEN_active_side == "b")
			counter_halfmove++;

		for (int n = 0; n < FEN_figures.length(); n++)
		{
			piece = 0;

			switch (FEN_figures[n])
			{
				case 'K': piece =  K; break;
				case 'k': piece = -K; break;
				case 'Q': piece =  Q; break;
				case 'q': piece = -Q; break;
				case 'B': piece =  B; break;
				case 'b': piece = -B; break;
				case 'N': piece =  N; break;
				case 'n': piece = -N; break;
				case 'R': piece =  R; break;
				case 'r': piece = -R; break;
				case 'P': piece =  P; break;
				case 'p': piece = -P; break;
				case '/': i++; j = 0; break;
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
					delta = FEN_figures[n] - '0';
					j += delta;
					break;
			}

			if (piece)
			{
				board[i][j] = piece;
				j++;
			}
		}
	}

	Vector2i get_xy_by_piece(int piece)
	{
		Vector2i piece_xy;
		piece_xy.x = abs(piece) - 1;

		if (piece > 0)
			piece_xy.y = ROW_WHITE;
		else
			piece_xy.y = ROW_BLACK;

		return piece_xy;
	}

	String get_chess_coords_by_xy(Vector2i xy)
	{
		String coords;

		coords += char(xy.x + 97);
		coords += char(56 - xy.y);

		return coords;
	}

	Vector2i get_xy_by_chess_coords(String coords)
	{
		Vector2i xy;

		xy.x = coords[0] - 97;
		xy.y = 56 - coords[1];

		return xy;
	}

	void draw_move()
	{
		Vector2i point = Mouse::getPosition(window);
		Vector2i piece_xy;
		piece_xy = get_xy_by_piece(piece_selected);

		s_piece.setTextureRect(IntRect(piece_xy.x*square_size, piece_xy.y*square_size, square_size, square_size));
		s_piece.setPosition(point.x - square_size/2, point.y - square_size/2);
		window.draw(s_piece);
	}

	void draw_pieces()
	{
		Vector2i piece_xy;
		int x, y;

		for (int i = 0; i < 8; ++i)
		{
			for (int j = 0; j < 8; ++j)
			{
				if (board[i][j])
				{
					piece_xy = get_xy_by_piece(board[i][j]);

					if (game_over && board[i][j] * move_side == K)
						s_piece.setTextureRect(IntRect(6*square_size, piece_xy.y*square_size, square_size, square_size));
					else
						s_piece.setTextureRect(IntRect(piece_xy.x*square_size, piece_xy.y*square_size, square_size, square_size));

					if (BLACKSIDE)
					{
						x = 7 - j;
						y = 7 - i;
					}
					else
					{
						x = j;
						y = i;
					}

					s_piece.setPosition(x*square_size, y*square_size);
					window.draw(s_piece);
				}
			}
		}
	}

	int get_piece_code(char piece)
	{
		switch (piece)
		{
			case 'p': return -P;
			case 'P': return P;
			case 'r': return -R;
			case 'R': return R;
			case 'n': return -N;
			case 'N': return N;
			case 'b': return -B;
			case 'B': return B;
			case 'q': return -Q;
			case 'Q': return Q;
			case 'k': return -K;
			case 'K': return K;
		}
	}

	char get_piece_char(int piece)
	{
		switch (piece)
		{
			case P:		return 'P';
			case R:		return 'R';
			case N:		return 'N';
			case B:		return 'B';
			case Q:		return 'Q';
			case K:		return 'K';
			case -P:	return 'p';
			case -R:	return 'r';
			case -N:	return 'n';
			case -B:	return 'b';
			case -Q:	return 'q';
			case -K:	return 'k';
		}
	}

	void undo_move()
	{
		std::string last_move, sfrom, sto, castle_side;
		std::stringstream s(myPGN);
		Vector2i from, to;

		int piece = 0;
		int piece_enemy = 0;
		int piece_promo = 0;
		int find_capture, find_promo, find_castle, find_enpassant;

		while (s >> last_move){}

		if (last_move == "") return;

		if (last_move == "Ke1g1~~")
		{
			board[7][4] = K;
			board[7][5] = 0;
			board[7][6] = 0;
			board[7][7] = R;
			castle_K = 1;
		}
		else if (last_move == "Ke1c1~~")
		{
			board[7][4] = K;
			board[7][2] = 0;
			board[7][3] = 0;
			board[7][0] = R;
			castle_Q = 1;
		}
		else if (last_move == "ke8g8~~")
		{
			board[0][4] = -K;
			board[0][5] = 0;
			board[0][6] = 0;
			board[0][7] = -R;
			castle_k = 1;
		}
		else if (last_move == "ke8c8~~")
		{
			board[0][4] = -K;
			board[0][2] = 0;
			board[0][3] = 0;
			board[0][0] = -R;
			castle_q = 1;
		}
		else
		{
			piece = get_piece_code(last_move[0]);

			sfrom = last_move.substr(1, 2);
			sto = last_move.substr(3, 2);
			from = get_xy_by_chess_coords(sfrom);
			to = get_xy_by_chess_coords(sto);

			find_capture = last_move.find('x');
			find_promo = last_move.find('=');
			find_castle = last_move.find('~');
			find_enpassant = last_move.find('&');

			if (find_capture!=-1)
			{
				piece_enemy = get_piece_code(last_move[find_capture+1]);
			}

			if (find_promo!=-1)
			{
				piece_promo = get_piece_code(last_move[find_promo+1]);
			}

			if (find_castle!=-1)
			{
				if (last_move[find_castle+1] == 'q')
					castle_Q = 1;
				else if (last_move[find_castle+1] == 'k')
					castle_K = 1;
				else
				{
					castle_K = 1;
					castle_Q = 1;
				}
			}

			if (find_enpassant!=-1)
			{
				piece_enemy = -piece;
				board[to.y][to.x] = 0;

				if (piece > 0)
					to.y += 1;
				else
					to.y -= 1;
			}

			board[from.y][from.x] = piece;
			board[to.y][to.x] = piece_enemy;
		}

		size_t len = UCI.getSize();
		if ( len >= 5)
		{
			if (piece_promo != 0)
				UCI.erase(len - 6, 6);
			else
				UCI.erase(len - 5, 5);
		}

		myPGN.erase(myPGN.find(last_move), last_move.length() + 1);

		if (game_over)
			game_over = false;

		move_side = -move_side;
		counter_halfmove--;

		if (counter_halfmove % 2 == 1)
			counter_move--;
	}

	void print_board()
	{
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
				std::cout << board[i][j] << "\t";
			std::cout << std::endl;
		}
	}

	bool is_square_checked_horizontally(Vector2i square)
	{
		bool rc = false;

		for (int i = square.x - 1; i >= 0; i--)
		{
			if (board[square.y][i] == 0)
				continue;

			if (board[square.y][i] == -move_side * R || board[square.y][i] == -move_side * Q)
				rc = true;

			break;
		}

		for (int i = square.x + 1; i <= 7; i++)
		{
			if (board[square.y][i] == 0)
				continue;

			if (board[square.y][i] == -move_side * R || board[square.y][i] == -move_side * Q)
				rc = true;

			break;
		}

		return rc;
	}

	bool is_square_checked_vertically(Vector2i square)
	{
		bool rc = false;

		for (int i = square.y - 1; i >= 0; i--)
		{
			if (board[i][square.x] == 0)
				continue;

			if (board[i][square.x] == -move_side * R || board[i][square.x] == -move_side * Q)
				rc = true;

			break;
		}

		for (int i = square.y + 1; i <= 7; i++)
		{
			if (board[i][square.x] == 0)
				continue;

			if (board[i][square.x] == -move_side * R || board[i][square.x] == -move_side * Q)
				rc = true;

			break;
		}

		return rc;
	}

	bool is_square_checked_diagonally(Vector2i square)
	{
		bool rc = false;

		for (int i = square.y - 1, j = square.x - 1; i >= 0, j >= 0; i--, j--)
		{
			if (board[i][j] == 0)
				continue;

			if (board[i][j] == -move_side * B || board[i][j] == -move_side * Q)
			{
				rc = true;
			}

			break;
		}

		for (int i = square.y - 1, j = square.x + 1; i >= 0, j <= 7; i--, j++)
		{
			if (board[i][j] == 0)
				continue;

			if (board[i][j] == -move_side * B || board[i][j] == -move_side * Q)
				rc = true;

			break;
		}

		for (int i = square.y + 1, j = square.x - 1; i <= 7, j >= 0; i++, j--)
		{
			if (board[i][j] == 0)
				continue;

			if (board[i][j] == -move_side * B || board[i][j] == -move_side * Q)
				rc = true;

			break;
		}

		for (int i = square.y + 1, j = square.x + 1; i <= 7, j <= 7; i++, j++)
		{
			if (board[i][j] == 0)
				continue;

			if (board[i][j] == -move_side * B || board[i][j] == -move_side * Q)
				rc = true;

			break;
		}

		return rc;
	}

	bool is_square_checked_by_knight(Vector2i square)
	{
		int x, y;

		y = square.y - 2;
		if (y >= 0)
		{
			x = square.x - 1;
			if (x >= 0)
				if (board[y][x] == -move_side * N)
					return true;

			x = square.x + 1;
			if (x <= 7)
				if (board[y][x] == -move_side * N)
					return true;
		}

		y = square.y - 1;
		if (y >= 0)
		{
			x = square.x - 2;
			if (x >= 0)
				if (board[y][x] == -move_side * N)
					return true;

			x = square.x + 2;
			if (x <= 7)
				if (board[y][x] == -move_side * N)
					return true;
		}

		y = square.y + 1;
		if (y <= 7)
		{
			x = square.x - 2;
			if (x >= 0)
				if (board[y][x] == -move_side * N)
					return true;

			x = square.x + 2;
			if (x <= 7)
				if (board[y][x] == -move_side * N)
					return true;
		}

		y = square.y + 2;
		if (y <= 7)
		{
			x = square.x - 1;
			if (x >= 0)
				if (board[y][x] == -move_side * N)
					return true;

			x = square.x + 1;
			if (x <= 7)
				if (board[y][x] == -move_side * N)
					return true;
		}

		return false;
	}

	bool is_square_checked_by_pawn(Vector2i square)
	{
		int x, y;

		std::cout << "King(x,y)=" << square.x << square.y << std::endl;
		y = square.y - move_side;

		x = square.x - 1;
		std::cout << "Pawn1(x,y)=" << x << y << std::endl;
		if (x >= 0 && y >= 0 && y <= 7)
			if (board[y][x] == -move_side * P)
				return true;

		x = square.x + 1;
		std::cout << "Pawn2(x,y)=" << x << y << std::endl;
		if (x <= 7 && y >= 0 && y <= 7)
			if (board[y][x] == -move_side * P)
				return true;

		return false;
	}

	bool is_line_empty(Vector2i from, Vector2i to)
	{
		int start_x = 0, stop_x = 0, start_y = 0, stop_y = 0;

		if (from == to)
			return true;

		if (from.x == to.x)
		{
			if (from.y < to.y)
			{
				start_y = from.y + 1;
				stop_y = to.y;
			}
			else
			{
				start_y = to.y + 1;
				stop_y = from.y;
			}

			for (int i = start_y; i < stop_y; i++)
				if (board[i][from.x] != 0)
					return false;
		}

		if (from.y == to.y)
		{
			if (from.x < to.x)
			{
				start_x = from.x + 1;
				stop_x = to.x;
			}
			else
			{
				start_x = to.x + 1;
				stop_x = from.x;
			}

			for (int i = start_x; i < stop_x; i++)
				if (board[from.y][i] != 0)
					return false;
		}

		if (abs(from.x - to.x) > 0 && abs(from.y - to.y) > 0)
		{
			if (abs(from.x - to.x) != abs(from.y - to.y)) return false;

			if ((from.y - to.y) * (from.x - to.x) > 0)
			{
				start_y = std::min(from.y, to.y) + 1;
				start_x = std::min(from.x, to.x) + 1;
				stop_y = std::max(from.y, to.y) - 1;
				stop_x = std::max(from.x, to.x) - 1;

				for (int i = 0; i <= stop_y - start_y; i++)
					if (board[start_y + i][start_x + i] != 0)
						return false;
			}
			else
			{
				start_y = std::min(from.y, to.y) + 1;
				start_x = std::max(from.x, to.x) - 1;
				stop_y = std::max(from.y, to.y) - 1;
				stop_x = std::min(from.x, to.x) + 1;

				for (int i = 0; i <= stop_y - start_y; i++)
					if (board[start_y + i][start_x - i] != 0)
						return false;
			}
		}

		return true;
	}

	bool check_enpassant_capture(Vector2i from, Vector2i to)
	{
		int y_captured;

		if (piece_selected == P && to.y == enpassant.y && to.x == enpassant.x && from.y == 3
			|| piece_selected == -P && to.y == enpassant.y && to.x == enpassant.x && from.y == 4)
		{
			if (piece_selected == P)
				y_captured = enpassant.y + 1;
			else if (piece_selected == -P)
				y_captured = enpassant.y - 1;

			board[y_captured][enpassant.x] = 0;
			return true;
		}
		else
			return false;
	}

	Vector2i find_king(int _board[8][8])
	{
		Vector2i square_king;

		for (int i = 0; i < 8; i++)
			for (int j = 0; j < 8; j++)
				if (_board[i][j] == move_side * K)
				{
					square_king.x = j;
					square_king.y = i;
				}

		return square_king;
	}

	bool check_king_checked2(int _board[8][8])
	{

		return false;
	}

	bool check_king_checked(Vector2i square_king)
	{
		bool rc = false;
		return rc;//--------------------------------------------!!!!!!!!!!!!!!!!!!!!!!!-------------------
		rc |= is_square_checked_horizontally(square_king);
		rc |= is_square_checked_vertically(square_king);
		rc |= is_square_checked_diagonally(square_king);
		rc |= is_square_checked_by_knight(square_king);
		rc |= is_square_checked_by_pawn(square_king);

		return rc;
	}

	bool check_move_king(Vector2i from, Vector2i to)
	{
		bool rc = true;

		if (abs(from.x - to.x) > 1)
		{
			if (piece_selected == K && castle_K == 1
				&& from.y == 7 && from.x == 4 && to.y == 7 && to.x == 6
				&& board[7][5] == 0 && board[7][6] == 0 && board[7][7] == R)
			{
				board[7][7] = 0;
				board[7][5] = R;
				castle_K = 0;
				rc = true;
			}
			else if (piece_selected == K && castle_Q == 1
				&& from.y == 7 && from.x == 4 && to.y == 7 && to.x == 2
				&& board[7][3] == 0 && board[7][2] == 0 && board[7][1] == 0 && board[7][0] == R)
			{
				board[7][0] = 0;
				board[7][3] = R;
				castle_Q = 0;
				rc = true;
			}
			else if (piece_selected == -K && castle_k == 1
				&& from.y == 0 && from.x == 4 && to.y == 0 && to.x == 6
				&& board[0][5] == 0 && board[0][6] == 0 && board[0][7] == -R)
			{
				board[0][7] = 0;
				board[0][5] = -R;
				castle_k = 0;
				rc = true;
			}
			else if (piece_selected == -K && castle_q == 1
				&& from.y == 0 && from.x == 4 && to.y == 0 && to.x == 2
				&& board[0][3] == 0 && board[0][2] == 0 && board[0][1] == 0 && board[0][0] == -R)
			{
				board[0][0] = 0;
				board[0][3] = -R;
				castle_q = 0;
				rc = true;
			}
			else
				rc = false;
		}
		else if (abs(from.y - to.y) <= 1 && abs(from.x - to.x) <= 1)
		{
			rc = is_line_empty(from, to);
			if (rc)
			{
				if (piece_selected == K)
				{
					castle_K = 0;
					castle_Q = 0;
				}
				if (piece_selected == -K)
				{
					castle_k = 0;
					castle_q = 0;
				}
			}
		}
		else
			rc = false;

		return rc;
	}

	bool check_move_pawn(Vector2i from, Vector2i to)
	{
		bool rc = true;

		if (check_enpassant_capture(from, to))
			return true;

		if (piece_selected == P)
		{
			if (from.y <= to.y
				|| abs(from.x - to.x) > 1
				|| abs(from.y - to.y) > 1 && to.y != 4
				|| abs(from.x - to.x) == 1 && (from.y != to.y + 1 || board[to.y][to.x] >= 0)
				|| from.x == to.x && from.y == to.y + 1 && board[to.y][to.x] != 0
				|| from.x == to.x && from.y == 6 && to.y == 4 && (board[5][to.x] != 0 || board[4][to.x] != 0))
				rc = false;
			else
			{
				if (to.y == 0)
					piece_selected = Q;

				if (from.x == to.x && from.y == 6 && to.y == 4 && board[5][to.x] == 0 && board[4][to.x] == 0)
				{
					enpassant.x = from.x;
					enpassant.y = 5;
				}
				else
				{
					enpassant.y = 0;
					enpassant.x = 0;
				}
			}
		}

		else if (piece_selected == -P)
		{
			if (from.y >= to.y
				|| abs(from.x - to.x) > 1
				|| abs(from.y - to.y) > 1 && to.y != 3
				|| abs(from.x - to.x) == 1 && (from.y != to.y - 1 || board[to.y][to.x] <= 0)
				|| from.x == to.x && from.y == to.y - 1 && board[to.y][to.x] != 0
				|| from.x == to.x && from.y == 1 && to.y == 3 && (board[2][to.x] != 0 || board[3][to.x] != 0))
				rc = false;
			else
			{
				if (to.y == 7)
					piece_selected = -Q;

				if (from.x == to.x && from.y == 1 && to.y == 3 && board[2][to.x] == 0 && board[3][to.x] == 0)
				{
					enpassant.x = from.x;
					enpassant.y = 2;
				}
				else
				{
					enpassant.y = 0;
					enpassant.x = 0;
				}
			}
		}

		return rc;
	}

	bool check_move_queen(Vector2i from, Vector2i to)
	{
		return is_line_empty(from, to);
	}

	bool check_move_rook(Vector2i from, Vector2i to)
	{
		bool rc = true;

		if (abs(from.x - to.x) > 0 && abs(from.y - to.y) > 0)
			rc = false;
		else
		{
			rc = is_line_empty(from, to);
			if (rc)
			{
				if (from.y == 7 && from.x == 0 && castle_Q == 1)
					castle_Q = 0;
				if (from.y == 7 && from.x == 7 && castle_K == 1)
					castle_K = 0;
				if (from.y == 0 && from.x == 0 && castle_q == 1)
					castle_q = 0;
				if (from.y == 0 && from.x == 7 && castle_k == 1)
					castle_k = 0;
			}
		}

		return rc;
	}

	bool check_move_bishop(Vector2i from, Vector2i to)
	{
		bool rc = true;

		if (from.x - to.x == 0 || from.y - to.y == 0)
			rc = false;
		else
			rc = is_line_empty(from, to);

		return rc;
	}

	bool check_move_knight(Vector2i from, Vector2i to)
	{
		bool rc = true;

		if (abs(from.x - to.x) == 2 && abs(from.y - to.y) == 1
			|| abs(from.x - to.x) == 1 && abs(from.y - to.y) == 2)
			rc = true;
		else
			rc = false;

		return rc;
	}

	bool check_move(Vector2i from, Vector2i to)
	{
		bool rc = true;
		bool king_checked = false;
		bool king_checked_after_move = false;
		Vector2i square_king;

		if (modify_board)
			return true;

		if (piece_selected != move_side * K)
		{
			square_king = find_king(board);
			king_checked = check_king_checked(square_king);
		}

		if (king_checked && piece_selected != move_side * K)
		{
			rc = false;
		}
		else
		{
			if (piece_selected * move_side < 0
				|| from == to
				|| piece_selected * board[to.y][to.x] > 0)
				rc = false;
			else
			{
				switch (piece_selected)
				{
					case P:
					case -P:
						rc = check_move_pawn(from, to);
						break;

					case K:
					case -K:
						rc = check_move_king(from, to);
						break;

					case Q:
					case -Q:
						rc = check_move_queen(from, to);
						break;

					case R:
					case -R:
						rc = check_move_rook(from, to);
						break;

					case B:
					case -B:
						rc = check_move_bishop(from, to);
						break;

					case N:
					case -N:
						rc = check_move_knight(from, to);
						break;
				}
			}
		}

		return rc;
	}

	bool check_mate()
	{
		std::string engine_move;

		engine_move = get_engine_move();

		if (engine_move == "(none")
			game_over = true;

		return game_over;
	}

	void make_engine_move()
	{
		int piece_enemy;
		int piece_selected_old = 0;
		Vector2i from, to;
		std::string engine_move;

		engine_move = get_engine_move();

		if (engine_move == "(none")
		{
			game_over = true;
			return;
		}

		from = get_xy_by_chess_coords(engine_move.substr(0, 2));
		to = get_xy_by_chess_coords(engine_move.substr(2, 2));

		piece_selected = board[from.y][from.x];
		piece_enemy = board[to.y][to.x];

		String sFrom = get_chess_coords_by_xy(from);
		String sTo = get_chess_coords_by_xy(to);

		piece_selected_old = piece_selected;
		check_move(from, to);

		board[to.y][to.x] = piece_selected;
		board[from.y][from.x] = 0;

		log_game(piece_selected_old, from, to, piece_enemy);

		counter_halfmove++;

		if (move_side == MOVE_SIDE_BLACK)
			counter_move++;

		move_side = -move_side;
	}

	void connect_engine(const char* path)
	{
		STARTUPINFOW su_info = {0};
		SECURITY_ATTRIBUTES sec_atts = {0};

		wchar_t wtext[80];
		LPWSTR ptr = wtext;
		mbstowcs(wtext, path, strlen(path) + 1);

		sec_atts.nLength = sizeof(sec_atts);
		sec_atts.bInheritHandle = TRUE;
		sec_atts.lpSecurityDescriptor = NULL;

		CreatePipe(&pipout_r, &pipout_w, &sec_atts, 0);
		CreatePipe(&pipin_r, &pipin_w, &sec_atts, 0);

		su_info.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
		su_info.wShowWindow = SW_HIDE;
		su_info.hStdInput = pipin_r;
		su_info.hStdOutput = pipout_w;
		su_info.hStdError = pipout_w;

		CreateProcessW(NULL, ptr, NULL, NULL, TRUE, 0, NULL, NULL, &su_info, &proc_info);
	}

	std::string get_engine_move()
	{
		BYTE buffer[2048];
		DWORD bytes, bytes_available;
		std::string str;
		std::string UCI_command;

		if (FEN.length())
			if (UCI.toAnsiString().length() == 0)
				UCI_command = "position fen " + FEN + "\ngo depth " + std::to_string(ENGINE_DEPTH) + "\n";
			else
				UCI_command = "position fen " + FEN + " moves " + UCI.toAnsiString() + "\ngo depth " + std::to_string(ENGINE_DEPTH) + "\n";

		else
			UCI_command = "position startpos moves " + UCI.toAnsiString() + "\ngo depth " + std::to_string(ENGINE_DEPTH) + "\n";

		WriteFile(pipin_w, UCI_command.c_str(), UCI_command.length(), &bytes, NULL);
		Sleep(500);
		PeekNamedPipe(pipout_r, buffer, sizeof(buffer), &bytes, &bytes_available, NULL);
		do
		{
			ZeroMemory(buffer, sizeof(buffer));
			if (!ReadFile(pipout_r, buffer, sizeof(buffer), &bytes, NULL) || !bytes) break;
			buffer[bytes] = 0;
			str += (char*)buffer;
		}
		while (bytes >= sizeof(buffer));
		int n = str.find("bestmove");

		if (n != -1)
			return str.substr(n + 9, 5);
		else
			return "error";
	}

	void close_connection()
	{
		DWORD bytes;

		WriteFile(pipin_w, "quit\n", 5, &bytes, NULL);
		if (pipin_w != NULL) CloseHandle(pipin_w);
		if (pipin_r != NULL) CloseHandle(pipin_r);
		if (pipout_w != NULL) CloseHandle(pipout_w);
		if (pipout_r != NULL) CloseHandle(pipout_r);
		if (proc_info.hProcess != NULL) CloseHandle(proc_info.hProcess);
		if (proc_info.hThread != NULL) CloseHandle(proc_info.hThread);
	}

	void log_game(int piece, Vector2i from, Vector2i to, int piece_enemy)
	{
		String fromS, toS;
		std::string sfrom, sto;

		fromS = get_chess_coords_by_xy(from);
		toS = get_chess_coords_by_xy(to);
		sfrom = fromS.toAnsiString();
		sto = toS.toAnsiString();

		UCI += fromS;
		UCI += toS;

		myPGN += get_piece_char(piece) + sfrom + sto;

		if (to == enpassant && enpassant.x != 0)
		{
			myPGN += "&";
		}

		if (piece * piece_enemy < 0)
		{
			myPGN += "x";
			myPGN += get_piece_char(piece_enemy);
		}

		if (piece == P && sto[1] == '8')
		{
			myPGN += "=Q";
			UCI += "q";
		}
		else if (piece == -P && sto[1] == '1')
		{
			myPGN += "=q";
			UCI += "q";
		}
		else if (piece == R && sfrom == "a1")
			myPGN += "~q";//потеря рокировки 0-0-0
		else if (piece == R && sfrom == "h1")
			myPGN += "~k";//потеря рокировки 0-0
		else if (piece == -R && sfrom == "a8")
			myPGN += "~q";//потеря рокировки 0-0-0
		else if (piece == -R && sfrom == "h8")
			myPGN += "~k";//потеря рокировки 0-0
		else if (piece == K || piece == -K)
			myPGN += "~~";//потеря обеих рокировок

		myPGN += " ";
		UCI += " ";
	}

	void run()
	{
		Event event;
		Vector2i mouse_xy, from, to;
		bool selected = false;
		bool engine_turn = false;
		int piece;
		int piece_enemy;
		std::string engine_move;

		if (PLAY_WITH_ENGINE == 1)
		{
			if (BLACKSIDE == 1 && move_side == MOVE_SIDE_WHITE
				|| BLACKSIDE == 0 && move_side == MOVE_SIDE_BLACK)
				engine_turn = true;
		}

		while (window.isOpen())
		{
			if (engine_turn)
			{
				make_engine_move();
				engine_turn = false;
				// check_mate();
			}

			while (window.pollEvent(event))
			{
				if (event.type == Event::Closed)
					window.close();

				if (event.type == Event::Resized)
					std::cout << event.size.width << ", " << event.size.height << std::endl;

				if (event.type == Event::KeyPressed)
				{
					if (event.key.code == Keyboard::Escape)
					{
						close_connection();
						window.close();
					}

					if (event.key.code == Keyboard::BackSpace)
						undo_move();

					if (event.key.code == Keyboard::Space)
						make_engine_move();

					if (event.key.code == Keyboard::P)
						std::cout << "myPGN = " << std::endl << myPGN << std::endl;

					if (event.key.code == Keyboard::U)
						std::cout << "UCI = " << std::endl << UCI.toAnsiString() << std::endl;

					if (event.key.code == Keyboard::F)
						std::cout << "FEN = " << std::endl << get_FEN() << std::endl;

					if (event.key.code == Keyboard::B)
						print_board();

					if (event.key.code == Keyboard::E)
					{
						play_with_engine = !play_with_engine;
						if (play_with_engine)
							engine_turn = true;
					}

					if (event.key.code == Keyboard::G)
					{
						engine_move = get_engine_move();
						std::cout << engine_move << std::endl;
					}

					if (event.key.code == Keyboard::Q)
					{						
						piece_selected = Q;
					}

					if (event.key.code == Keyboard::M)
					{

						if (modify_board)
						{
							std::cout << "Режим редактирования выключён" << std::endl;
							play_with_engine = 1;
							FEN = get_FEN();
							UCI = "";
							myPGN = "";
						}
						else
						{
							std::cout << "Режим редактирования включён" << std::endl;
							play_with_engine = 0;
							enpassant.x = enpassant.y = 0;
						}
						modify_board = !modify_board;
					}

					// if (event.key.code == Keyboard::C)
					// {
					// 	Vector2i square;
					// 	Vector2i square_king;
					// 	square_king = find_king();

					// 	if (is_square_checked_by_pawn(square_king))
					// 	{
					// 		std::cout << "Checked by pawn: " << get_chess_coords_by_xy(square).toAnsiString() << std::endl;
					// 	}
					// 	else
					// 		std::cout << "pawned" << std::endl;

						// for (int i = 0; i < 8; ++i)
						// {
						// 	for (int j = 0; j < 8; ++j)
						// 	{
						// 		square.y = i;
						// 		square.x = j;

								// if (is_square_checked_by_knight(square))
								// {
								// 	std::cout << "Checked by knight: " << get_chess_coords_by_xy(square).toAnsiString() << std::endl;
								// }

								// if (is_square_checked_horizontally(square))
								// {
								// 	std::cout << "Checked horizontally: " << get_chess_coords_by_xy(square).toAnsiString() << std::endl;
								// }

								// if (is_square_checked_vertically(square))
								// {
								// 	std::cout << "Checked vertically: " << get_chess_coords_by_xy(square).toAnsiString() << std::endl;
								// }
								// if (is_square_checked_diagonally(square))
								// {
								// 	std::cout << "Checked horizontally: " << get_chess_coords_by_xy(square).toAnsiString() << std::endl;
								// }
						// 	}
						// }
					// }
				}

				if (event.type == Event::MouseButtonPressed)
				{
					if (event.key.code == (Keyboard::Key)Mouse::Left)
					{
						from = Mouse::getPosition(window);

						from.x /= square_size;
						from.y /= square_size;

						if (BLACKSIDE)
						{
							from.x = 7 - from.x;
							from.y = 7 - from.y;
						}

						piece_selected = board[from.y][from.x];

						if (piece_selected)
						{
							board[from.y][from.x] = 0;
							selected = true;
						}
					}
				}

				if (event.type == Event::MouseButtonReleased && selected == true)
				{
					if (event.key.code == (Keyboard::Key)Mouse::Left)
					{
						selected = false;
						to = Mouse::getPosition(window);
						to.x /= square_size;
						to.y /= square_size;

						if (BLACKSIDE)
						{
							to.x = 7 - to.x;
							to.y = 7 - to.y;
						}

						piece = piece_selected;
						if (check_move(from, to))
						{
							piece_enemy = board[to.y][to.x];
							board[to.y][to.x] = piece_selected;
							if (piece_selected != P && piece_selected != -P)
							{
								enpassant.y = 0;
								enpassant.x = 0;
							}

							if (modify_board == false)
							{
								log_game(piece, from, to, piece_enemy);
								counter_halfmove++;
								if (move_side == MOVE_SIDE_BLACK)
									counter_move++;
								move_side = -move_side;

								if (play_with_engine)
									engine_turn = true;
							}
						}
						else
							board[from.y][from.x] = piece_selected;
					}
				}
			}

			window.clear();
			window.draw(s_board);
			draw_pieces();

			if (selected)
				draw_move();

			window.display();
		}
	}
};

int main(int argc, char const *argv[])
{
	Chess c;

	// startpos:
	// c.set_FEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

	// e2e4:
	// c.set_FEN("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1 ");

	// enpassant: d2d4 e7e5 d4d5 c7c5
	// c.set_FEN("rnbqkbnr/pp1p1ppp/8/2pPp3/8/8/PPP1PPPP/RNBQKBNR w KQkq c6 0 3");

	//Reti
	// c.set_FEN("7K/8/k1P5/7p/8/8/8/8 w - - 0 1");

	// c.set_FEN("6k1/4Rpb1/6pp/1Np5/8/7P/2P2nPK/3r4 b - - 0 31");
	// c.set_FEN("k7/RR6/8/8/8/8/K/8 w - - 0 32");
	// c.set_FEN("4Q3/2b4r/7B/6R1/5k/8/7K/5q2");
	// c.set_FEN("7k/P7/8/8/8/8/8/K7");

	c.run();

	return 0;
}