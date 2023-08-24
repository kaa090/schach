//доработать:
// - проверка короля на шах
// - проверка рокировки
// - отмена хода, возврат взятых фигур
// - отмена хода, возврат права рокировки
// - подсветка последнего хода
// - подсветка шаха и мата
// - stockfish для linux
// - выгрузка FEN
// - смена сторон
// - редактор

#include <iostream>
#include <SFML/Graphics.hpp>

#include <windows.h>
#include <stdio.h>
#include <string>
#include <sstream>

using namespace sf;

const bool PLAY_WITH_ENGINE = 1;
const int ENGINE_DEPTH = 1; // [1, 20]
const int CHESS_SIZE = 0; // 0 - small, 1 - big

//CHESS_SIZE = 0:
const int CELL_SIZE_0 = 50;
const char* PATH_BOARD_0 = "rsc/small_board.png";
const char* PATH_PIECES_0 = "rsc/small_pieces.png";

//CHESS_SIZE = 1:
const int CELL_SIZE_1 = 100;
const char* PATH_BOARD_1 = "rsc/big_board.png";
const char* PATH_PIECES_1 = "rsc/big_pieces.png";

const char* PATH_ENGINE = "D:\\prog\\cpp\\schach\\stockfish\\stockfish.exe";

const int K = 1;
const int Q = 2;
const int B = 3;
const int N = 4;
const int R = 5;
const int P = 6;
const int ROW_WHITE = 0;
const int ROW_BLACK = 1;

class Chess
{
	int cell_size;
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
	std::string PGN;
	std::string FEN;

	int counter_halfmove;
	int piece_selected;
	int flag_turn;
	int flag_castle_00_white, flag_castle_000_white, flag_castle_00_black, flag_castle_000_black;
	Vector2i enpassant;

	bool play_with_engine;
	PROCESS_INFORMATION proc_info;
	HANDLE pipin_w, pipin_r, pipout_w, pipout_r;

public:

	Chess()
	{
		if(CHESS_SIZE == 0)
		{
			cell_size = CELL_SIZE_0;
			t_board.loadFromFile(PATH_BOARD_0);
			t_pieces.loadFromFile(PATH_PIECES_0);
		}
		else if(CHESS_SIZE == 1)
		{
			cell_size = CELL_SIZE_1;
			t_board.loadFromFile(PATH_BOARD_1);
			t_pieces.loadFromFile(PATH_PIECES_1);
		}

		s_board.setTexture(t_board);
		s_piece.setTexture(t_pieces);
		window.create(VideoMode(8*cell_size, 8*cell_size), "kaaChess");
		window.setFramerateLimit(60);

		counter_halfmove = 0;
		flag_turn = 1;
		flag_castle_00_white = 1;
		flag_castle_000_white = 1;
		flag_castle_00_black = 1;
		flag_castle_000_black = 1;

		proc_info = {0};
		connect_engine(PATH_ENGINE);

		play_with_engine = PLAY_WITH_ENGINE;
	}

	void clear_board()
	{
		for(int i = 0; i < 8; ++i)
			for(int j = 0; j < 8; ++j)
				board[i][j] = 0;
	}

	void set_FEN(std::string _FEN)
	{
		int i = 0, j = 0;
		int piece;
		int delta;

		clear_board();

		FEN = _FEN;

		for(int n = 0; n < _FEN.length(); n++)
		{
			piece = 0;

			switch(_FEN[n]){
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
					delta = _FEN[n] - '0';
					j += delta;
					break;
			}

			if(piece) 
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

		if(piece > 0) piece_xy.y = ROW_WHITE;
		else piece_xy.y = ROW_BLACK;

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

		s_piece.setTextureRect(IntRect(piece_xy.x * cell_size, piece_xy.y * cell_size, cell_size, cell_size));
		s_piece.setPosition(point.x - cell_size/2, point.y - cell_size/2);
		window.draw(s_piece);
	}

	void draw_pieces()
	{
		Vector2i piece_xy;

		for(int i = 0; i < 8; ++i)
		{
			for(int j = 0; j < 8; ++j)
			{
				if(board[i][j])
				{
					piece_xy = get_xy_by_piece(board[i][j]);

					s_piece.setTextureRect(IntRect(piece_xy.x*cell_size, piece_xy.y*cell_size, cell_size, cell_size));
					s_piece.setPosition(j*cell_size, i*cell_size);
					window.draw(s_piece);
				}
			}
		}
	}

	int get_piece_code(char piece)
	{
		switch(piece)
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
		switch(piece)
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

	void move_back()
	{
		std::string last_move, sfrom, sto, castle_side;
		std::stringstream s(PGN);
		Vector2i from, to;

		int piece = 0;
		int piece_enemy = 0;
		int piece_promo = 0;
		int find_x;
		int find_y;
		int find_z;

		while(s >> last_move){}

		if(last_move == "") return;

		if(last_move == "Ke1g1~~")
		{
			board[7][4] = K;
			board[7][5] = 0;
			board[7][6] = 0;
			board[7][7] = R;
			flag_castle_00_white = 1;
		}
		else if(last_move == "Ke1c1~~")
		{
			board[7][4] = K;
			board[7][2] = 0;
			board[7][3] = 0;
			board[7][0] = R;
			flag_castle_000_white = 1;
		}
		else if(last_move == "ke8g8~~")
		{
			board[0][4] = -K;
			board[0][5] = 0;
			board[0][6] = 0;
			board[0][7] = -R;
			flag_castle_00_black = 1;
		}
		else if(last_move == "ke8c8~~")
		{
			board[0][4] = -K;
			board[0][2] = 0;
			board[0][3] = 0;
			board[0][0] = -R;
			flag_castle_000_black = 1;
		}
		else
		{
			piece = get_piece_code(last_move[0]);

			sfrom = last_move.substr(1, 2);
			sto = last_move.substr(3, 2);
			from = get_xy_by_chess_coords(sfrom);
			to = get_xy_by_chess_coords(sto);

			find_x = last_move.find('x');
			find_y = last_move.find('=');
			find_z = last_move.find('~');

			if(find_x!=-1)
			{
				piece_enemy = get_piece_code(last_move[find_x+1]);
			}

			if(find_y!=-1)
			{
				piece_promo = get_piece_code(last_move[find_y+1]);
			}

			if(find_z!=-1)
			{
				if(last_move[find_z+1] == 'q')
					flag_castle_000_white = 1;
				else if(last_move[find_z+1] == 'k')
					flag_castle_00_white = 1;
				else
				{
					flag_castle_00_white = 1;
					flag_castle_000_white = 1;
				}
			}

			board[from.y][from.x] = piece;
			board[to.y][to.x] = piece_enemy;
		}

		size_t len = UCI.getSize();
		if( len >= 5) UCI.erase(len - 5, 5);

		PGN.erase(PGN.find(last_move), last_move.length());

		flag_turn = -flag_turn;
		counter_halfmove--;
	}

	void move_back_UCI()
	{
		Vector2i from, to;
		size_t len;

		len = UCI.getSize();

		if( len >= 5)
		{
			from = get_xy_by_chess_coords(UCI.substring(len - 5, 2).toAnsiString());
			to = get_xy_by_chess_coords(UCI.substring(len - 3, 2).toAnsiString());
			piece_selected = board[to.y][to.x];
			board[from.y][from.x] = piece_selected;
			board[to.y][to.x] = 0;

			if(piece_selected == K && UCI.substring(len-5, 4).toAnsiString() == "e1g1")
			{
				board[7][5] = 0;
				board[7][7] = R;
				flag_castle_00_white = 1;
			}

			if(piece_selected == K && UCI.substring(len-5, 4).toAnsiString() == "e1c1")
			{
				board[7][3] = 0;
				board[7][0] = R;
				flag_castle_000_white = 1;
			}

			if(piece_selected == -K && UCI.substring(len-5, 4).toAnsiString() == "e8g8")
			{
				board[0][5] = 0;
				board[0][7] = -R;
				flag_castle_00_black = 1;
			}

			if(piece_selected == -K && UCI.substring(len-5, 4).toAnsiString() == "e8c8")
			{
				board[0][3] = 0;
				board[0][0] = -R;
				flag_castle_000_black = 1;
			}

			UCI.erase(len - 5, 5);
			flag_turn = -flag_turn;
			counter_halfmove--;
		}
	}

	void print_board()
	{
		for(int i = 0; i < 8; ++i)
		{
			for(int j = 0; j < 8; ++j)
				std::cout << board[i][j] << "\t";
			std::cout << std::endl;
		}
	}

	bool is_line_empty(Vector2i from, Vector2i to)
	{
		int start_x = 0, stop_x = 0, start_y = 0, stop_y = 0;

		if(from == to) 
			return true;

		if(from.x == to.x)
		{
			if(from.y < to.y)
			{
				start_y = from.y + 1;
				stop_y = to.y;
			}
			else
			{
				start_y = to.y + 1;
				stop_y = from.y;
			}

			for(int i = start_y; i < stop_y; i++)
				if(board[i][from.x] != 0)
					return false;
		}

		if(from.y == to.y)
		{
			if(from.x < to.x)
			{
				start_x = from.x + 1;
				stop_x = to.x;
			}
			else
			{
				start_x = to.x + 1;
				stop_x = from.x;
			}

			for(int i = start_x; i < stop_x; i++)
				if(board[from.y][i] != 0)
					return false;
		}

		if(abs(from.x - to.x) > 0 && abs(from.y - to.y) > 0)
		{
			if(abs(from.x - to.x) != abs(from.y - to.y)) return false;

			if((from.y - to.y) * (from.x - to.x) > 0)
			{
				start_y = std::min(from.y, to.y) + 1;
				start_x = std::min(from.x, to.x) + 1;
				stop_y = std::max(from.y, to.y) - 1;
				stop_x = std::max(from.x, to.x) - 1;

				for(int i = 0; i <= stop_y - start_y; i++)
					if(board[start_y + i][start_x + i] != 0)
						return false;
			}
			else
			{
				start_y = std::min(from.y, to.y) + 1;
				start_x = std::max(from.x, to.x) - 1;
				stop_y = std::max(from.y, to.y) - 1;
				stop_x = std::min(from.x, to.x) + 1;

				for(int i = 0; i <= stop_y - start_y; i++)
					if(board[start_y + i][start_x - i] != 0)
						return false;
			}
		}

		return true;
	}

	bool check_enpassant_capture(Vector2i from, Vector2i to)
	{
		int y_captured;

		if(piece_selected == P && to.y == enpassant.y && to.x == enpassant.x && from.y == 3
			|| piece_selected == -P && to.y == enpassant.y && to.x == enpassant.x && from.y == 4)
		{
			if(piece_selected == P)
				y_captured = enpassant.y + 1;
			else if(piece_selected == -P)
				y_captured = enpassant.y - 1;

			board[y_captured][enpassant.x] = 0;
			return true;
		}
		else
			return false;
	}

	bool check_king_checked(Vector2i from, Vector2i to)
	{
		bool rc = false;

		return rc;
	}

	bool check_move_pawn(Vector2i from, Vector2i to)
	{
		bool rc = true;

		if(check_enpassant_capture(from, to))
			return true;

		if(piece_selected == P)
		{
			if(from.y <= to.y 
				|| abs(from.x - to.x) > 1
				|| abs(from.y - to.y) > 1 && to.y != 4
				|| abs(from.x - to.x) == 1 && (from.y != to.y + 1 || board[to.y][to.x] >= 0)
				|| from.x == to.x && from.y == to.y + 1 && board[to.y][to.x] != 0
				|| from.x == to.x && from.y == 6 && to.y == 4 && (board[5][to.x] != 0 || board[4][to.x] != 0))
				rc = false;
			else
			{
				if(to.y == 0)
					piece_selected = Q;

				if(from.x == to.x && from.y == 6 && to.y == 4 && board[5][to.x] == 0 && board[4][to.x] == 0)
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

		else if(piece_selected == -P)
		{
			if(from.y >= to.y 
				|| abs(from.x - to.x) > 1
				|| abs(from.y - to.y) > 1 && to.y != 3
				|| abs(from.x - to.x) == 1 && (from.y != to.y - 1 || board[to.y][to.x] <= 0)
				|| from.x == to.x && from.y == to.y - 1 && board[to.y][to.x] != 0
				|| from.x == to.x && from.y == 1 && to.y == 3 && (board[2][to.x] != 0 || board[3][to.x] != 0))
				rc = false;
			else
			{
				if(to.y == 7)
					piece_selected = -Q;

				if(from.x == to.x && from.y == 1 && to.y == 3 && board[2][to.x] == 0 && board[3][to.x] == 0)
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

	bool check_move_king(Vector2i from, Vector2i to)
	{
		bool rc = true;

		if(check_king_checked(from, to))
			return false;

		if(abs(from.x - to.x) > 1)
		{
			if(piece_selected == K && flag_castle_00_white == 1
				&& from.y == 7 && from.x == 4 && to.y == 7 && to.x == 6
				&& board[7][5] == 0 && board[7][6] == 0 && board[7][7] == R)
			{
				board[7][7] = 0;
				board[7][5] = R;
				flag_castle_00_white = 0;
				rc = true;
			}
			else if(piece_selected == K && flag_castle_000_white == 1
				&& from.y == 7 && from.x == 4 && to.y == 7 && to.x == 2
				&& board[7][3] == 0 && board[7][2] == 0 && board[7][1] == 0 && board[7][0] == R)
			{
				board[7][0] = 0;
				board[7][3] = R;
				flag_castle_000_white = 0;
				rc = true;
			}
			else if(piece_selected == -K && flag_castle_00_black == 1
				&& from.y == 0 && from.x == 4 && to.y == 0 && to.x == 6
				&& board[0][5] == 0 && board[0][6] == 0 && board[0][7] == -R)
			{
				board[0][7] = 0;
				board[0][5] = -R;
				flag_castle_00_black = 0;
				rc = true;
			}
			else if(piece_selected == -K && flag_castle_000_black == 1
				&& from.y == 0 && from.x == 4 && to.y == 0 && to.x == 2
				&& board[0][3] == 0 && board[0][2] == 0 && board[0][1] == 0 && board[0][0] == -R)
			{
				board[0][0] = 0;
				board[0][3] = -R;
				flag_castle_000_black = 0;
				rc = true;
			}
			else
				rc = false;
		}
		else if(abs(from.y - to.y) <= 1 && abs(from.x - to.x) <= 1)
		{
			rc = is_line_empty(from, to);
			if(rc)
			{
				if(piece_selected == K)
				{
					flag_castle_00_white = 0;
					flag_castle_000_white = 0;
				}
				if(piece_selected == -K)
				{
					flag_castle_00_black = 0;
					flag_castle_000_black = 0;
				}
			}
		}
		else
			rc = false;

		return rc;
	}

	bool check_move_queen(Vector2i from, Vector2i to)
	{
		return is_line_empty(from, to);
	}

	bool check_move_rook(Vector2i from, Vector2i to)
	{
		bool rc = true;

		if(abs(from.x - to.x) > 0 && abs(from.y - to.y) > 0)
			rc = false;
		else
		{
			rc = is_line_empty(from, to);
			if(rc)
			{
				if(from.y == 7 && from.x == 0 && flag_castle_000_white == 1)
					flag_castle_000_white = 0;
				if(from.y == 7 && from.x == 7 && flag_castle_00_white == 1)
					flag_castle_00_white = 0;
				if(from.y == 0 && from.x == 0 && flag_castle_000_black == 1)
					flag_castle_000_black = 0;
				if(from.y == 0 && from.x == 7 && flag_castle_00_black == 1)
					flag_castle_00_black = 0;
			}
		}

		return rc;
	}

	bool check_move_bishop(Vector2i from, Vector2i to)
	{
		bool rc = true;

		if(from.x - to.x == 0 || from.y - to.y == 0)
			rc = false;
		else
			rc = is_line_empty(from, to);

		return rc;
	}

	bool check_move_knight(Vector2i from, Vector2i to)
	{
		bool rc = true;

		if(abs(from.x - to.x) == 2 && abs(from.y - to.y) == 1 
			|| abs(from.x - to.x) == 1 && abs(from.y - to.y) == 2)
			rc = true;
		else
			rc = false;

		return rc;
	}

	bool check_move(Vector2i from, Vector2i to)
	{
		bool rc = true;

		if(piece_selected * flag_turn < 0
			|| from == to
			|| piece_selected * board[to.y][to.x] > 0)
			rc = false;
		else
		{
			switch(piece_selected)
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
		return rc;
	}

	void make_engine_move()
	{
		int piece, piece_enemy;
		Vector2i from, to;
		std::string engine_move;

		engine_move = get_next_move(UCI.toAnsiString());

		from = get_xy_by_chess_coords(engine_move.substr(0, 2));
		to = get_xy_by_chess_coords(engine_move.substr(2, 2));
		piece = board[from.y][from.x];
		piece_enemy = board[to.y][to.x];
		board[to.y][to.x] = board[from.y][from.x];
		board[from.y][from.x] = 0;

		log_game(piece, from, to, piece_enemy);
		flag_turn = -flag_turn;
		counter_halfmove++;
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

	std::string get_next_move(std::string position)
	{
		BYTE buffer[2048];
		DWORD bytes, bytes_available;
		std::string str;

		position = "position startpos moves " + position + "\ngo depth " + std::to_string(ENGINE_DEPTH) + "\n";

		WriteFile(pipin_w, position.c_str(), position.length(), &bytes, NULL);
		Sleep(500);
		PeekNamedPipe(pipout_r, buffer, sizeof(buffer), &bytes, &bytes_available, NULL);
		do
		{
			ZeroMemory(buffer, sizeof(buffer));
			if(!ReadFile(pipout_r, buffer, sizeof(buffer), &bytes, NULL) || !bytes) break;
			buffer[bytes] = 0;
			str += (char*)buffer;
		}
		while(bytes >= sizeof(buffer));

		int n = str.find("bestmove");
		if (n != -1)
			return str.substr(n + 9, 4);
		else
			return "error";
	}

	void close_connection()
	{
		DWORD bytes;

		WriteFile(pipin_w, "quit\n", 5, &bytes, NULL);
		if(pipin_w != NULL) CloseHandle(pipin_w);
		if(pipin_r != NULL) CloseHandle(pipin_r);
		if(pipout_w != NULL) CloseHandle(pipout_w);
		if(pipout_r != NULL) CloseHandle(pipout_r);
		if(proc_info.hProcess != NULL) CloseHandle(proc_info.hProcess);
		if(proc_info.hThread != NULL) CloseHandle(proc_info.hThread);
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
		UCI += " ";

		PGN += get_piece_char(piece) + sfrom + sto;

		if(piece * piece_enemy < 0)
		{
			PGN += "x";
			PGN += get_piece_char(piece_enemy);
		}

		if(piece == P && sto[1] == '8')
			PGN += "=Q";
		else if(piece == -P && sto[1] == '1')
			PGN += "=q";
		else if(piece == R && sfrom == "a1")
			PGN += "~q";
		else if(piece == R && sfrom == "h1")
			PGN += "~k";
		else if(piece == -R && sfrom == "a8")
			PGN += "~q";
		else if(piece == -R && sfrom == "h8")
			PGN += "~k";
		else if(piece == K || piece == -K)
			PGN += "~~";

		PGN += " ";
	}

	void run()
	{
		Vector2i mouse_xy, from, to;
		bool selected = false;
		bool engine_turn = false;
		int piece;
		int piece_enemy;

		Event event;

		while(window.isOpen())
		{
			if(engine_turn)
			{
				make_engine_move();
				engine_turn = false;
			}

			while(window.pollEvent(event))
			{
				if(event.type == Event::Closed) window.close();

				if(event.type == Event::Resized)
				{
					std::cout << event.size.width << ", " << event.size.height << std::endl;
				}

				if(event.type == Event::KeyPressed)
				{
					if(event.key.code == Keyboard::Escape) 
						window.close();

					if(event.key.code == Keyboard::BackSpace)
						move_back();

					if(event.key.code == Keyboard::Space)
						make_engine_move();

					if(event.key.code == Keyboard::P)
						std::cout << PGN << std::endl;

					if(event.key.code == Keyboard::E)
					{
						play_with_engine = !play_with_engine;
						if(play_with_engine)
							engine_turn = true;
					}
				}

				if(event.type == Event::MouseButtonPressed)
				{
					if(event.key.code == (Keyboard::Key)Mouse::Left)
					{
						selected = true;
						from = Mouse::getPosition(window);
						from.x /= cell_size;
						from.y /= cell_size;
						piece_selected = board[from.y][from.x];
						board[from.y][from.x] = 0;
					}
				}

				if(event.type == Event::MouseButtonReleased)
				{
					if(event.key.code == (Keyboard::Key)Mouse::Left)
					{
						selected = false;
						to = Mouse::getPosition(window);
						to.x /= cell_size;
						to.y /= cell_size;

						piece = piece_selected;
						if(check_move(from, to))
						{
							piece_enemy = board[to.y][to.x];
							board[to.y][to.x] = piece_selected;
							if (piece_selected != P && piece_selected != -P)
							{
								enpassant.y = 0;
								enpassant.x = 0;
							}

							log_game(piece, from, to, piece_enemy);
							flag_turn = -flag_turn;
							counter_halfmove++;

							if(play_with_engine)
								engine_turn = true;
						}
						else 
							board[from.y][from.x] = piece_selected;
					}
				}
			}

			window.clear();
			window.draw(s_board);
			draw_pieces();

			if(selected)
			{
				draw_move();
			}

			window.display();
		}
	}
};

int main(int argc, char const *argv[])
{
	Chess c;

	c.set_FEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
	// c.set_FEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	// c.set_FEN("6k1/4Rpb1/6pp/1Np5/8/7P/2P2nPK/3r4 w - - 0 31");
	// c.set_FEN("4Q3/2b4r/7B/6R1/5k/8/7K/5q2");

	c.run();

	return 0;
}
