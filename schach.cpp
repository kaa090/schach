//доработать:
// - проверка короля на шах
// - проверка рокировки
// - отмена хода, возврат взятых фигур
// - отмена хода, возврат права рокироки
// - подсветка последнего хода
// - подсветка шаха и мата
// - stockfish для linux
// - выгрузка FEN
// - выгрузка PGN
// - смена сторон
// - редактор

#include <iostream>
#include <SFML/Graphics.hpp>

#include <windows.h>
#include <stdio.h>
#include <string>

using namespace sf;

const int CELL_SIZE = 100;
const int K = 1;
const int Q = 2;
const int B = 3;
const int N = 4;
const int R = 5;
const int P = 6;
const int ROW_WHITE = 0;
const int ROW_BLACK = 1;

const char* ENGINE_PATH = "D:\\prog\\cpp\\schach\\stockfish\\stockfish.exe";
const int ENGINE_DEPTH = 4;

class Chess
{
	String UCI;
	RenderWindow window;
	Sprite s_board, s_figure;
	Texture t_board, t_figures;

	// int board[8][8];
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

	int counter_halfmove;
	int figure_selected;
	int flag_turn;
	int flag_castle_00_white, flag_castle_000_white, flag_castle_00_black, flag_castle_000_black;
	Vector2i enpassant;

	PROCESS_INFORMATION proc_info;
	HANDLE pipin_w, pipin_r, pipout_w, pipout_r;

public:

	Chess()
	{
		t_board.loadFromFile("rsc/board.png");
		t_figures.loadFromFile("rsc/figures.png");

		s_board.setTexture(t_board);
		s_figure.setTexture(t_figures);
		window.create(VideoMode(800, 800), "kaaChess");
		window.setFramerateLimit(60);

		flag_turn = 1;
		flag_castle_00_white = 1;
		flag_castle_000_white = 1;
		flag_castle_00_black = 1;
		flag_castle_000_black = 1;

		proc_info = {0};
		connect_engine(ENGINE_PATH);
	}

	void clear_board()
	{
		for(int i = 0; i < 8; ++i)
		{
			for(int j = 0; j < 8; ++j)
			{
				board[i][j] = 0;
			}
		}
	}

	void set_FEN(String FEN)
	{
		int i = 0, j = 0;
		int figure;
		int delta;

		clear_board();

		for(int n = 0; n < FEN.getSize(); n++)
		{
			figure = 0;

			switch(FEN[n]){
				case 'K': figure =  K; break;
				case 'k': figure = -K; break;
				case 'Q': figure =  Q; break;
				case 'q': figure = -Q; break;
				case 'B': figure =  B; break;
				case 'b': figure = -B; break;
				case 'N': figure =  N; break;
				case 'n': figure = -N; break;
				case 'R': figure =  R; break;
				case 'r': figure = -R; break;
				case 'P': figure =  P; break;
				case 'p': figure = -P; break;
				case '/': i++; j = 0; break;
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
					delta = FEN[n] - '0';
					j += delta;
					break;
			}

			if(figure) 
			{
				board[i][j] = figure;
				j++;
			}
		}
	}

	Vector2i get_xy_by_figure(int figure)
	{
		Vector2i figure_xy;
		figure_xy.x = abs(figure) - 1;

		if(figure > 0) figure_xy.y = ROW_WHITE;
		else figure_xy.y = ROW_BLACK;

		return figure_xy;
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
		Vector2i figure_xy;
		figure_xy = get_xy_by_figure(figure_selected);

		s_figure.setTextureRect(IntRect(figure_xy.x * CELL_SIZE, figure_xy.y * CELL_SIZE, CELL_SIZE, CELL_SIZE));
		s_figure.setPosition(point.x - CELL_SIZE/2, point.y - CELL_SIZE/2);
		window.draw(s_figure);
	}

	void draw_figures()
	{
		Vector2i figure_xy;

		for(int i = 0; i < 8; ++i)
		{
			for(int j = 0; j < 8; ++j)
			{
				if(board[i][j])
				{
					figure_xy = get_xy_by_figure(board[i][j]);

					s_figure.setTextureRect(IntRect(figure_xy.x*CELL_SIZE, figure_xy.y*CELL_SIZE, CELL_SIZE, CELL_SIZE));
					s_figure.setPosition(j*CELL_SIZE, i*CELL_SIZE);
					window.draw(s_figure);
				}
			}
		}
	}

	void move_back()
	{
		Vector2i from, to;
		size_t len;

		len = UCI.getSize();

		if( len >= 5)
		{
			from = get_xy_by_chess_coords(UCI.substring(len - 5, 2).toAnsiString());
			to = get_xy_by_chess_coords(UCI.substring(len - 3, 2).toAnsiString());
			figure_selected = board[to.y][to.x];
			board[from.y][from.x] = figure_selected;
			board[to.y][to.x] = 0;

			if(figure_selected == K && UCI.substring(len-5, 4).toAnsiString() == "e1g1")
			{
				board[7][5] = 0;
				board[7][7] = R;
				flag_castle_00_white = 1;
			}

			if(figure_selected == K && UCI.substring(len-5, 4).toAnsiString() == "e1c1")
			{
				board[7][3] = 0;
				board[7][0] = R;
				flag_castle_000_white = 1;
			}

			if(figure_selected == -K && UCI.substring(len-5, 4).toAnsiString() == "e8g8")
			{
				board[0][5] = 0;
				board[0][7] = -R;
				flag_castle_00_black = 1;
			}

			if(figure_selected == -K && UCI.substring(len-5, 4).toAnsiString() == "e8c8")
			{
				board[0][3] = 0;
				board[0][0] = -R;
				flag_castle_000_black = 1;
			}

			UCI.erase(len - 5, 5);
			flag_turn = -flag_turn;
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

		if(figure_selected == P && to.y == enpassant.y && to.x == enpassant.x && from.y == 3
			|| figure_selected == -P && to.y == enpassant.y && to.x == enpassant.x && from.y == 4)
		{
			if(figure_selected == P)
				y_captured = enpassant.y + 1;
			else if(figure_selected == -P)
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

		if(figure_selected == P)
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
					figure_selected = Q;

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

		else if(figure_selected == -P)
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
					figure_selected = -Q;

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
			if(figure_selected == K && flag_castle_00_white == 1
				&& from.y == 7 && from.x == 4 && to.y == 7 && to.x == 6
				&& board[7][5] == 0 && board[7][6] == 0 && board[7][7] == R)
			{
				board[7][7] = 0;
				board[7][5] = R;
				flag_castle_00_white = 0;
				rc = true;
			}
			else if(figure_selected == K && flag_castle_000_white == 1
				&& from.y == 7 && from.x == 4 && to.y == 7 && to.x == 2
				&& board[7][3] == 0 && board[7][2] == 0 && board[7][1] == 0 && board[7][0] == R)
			{
				board[7][0] = 0;
				board[7][3] = R;
				flag_castle_000_white = 0;
				rc = true;
			}
			else if(figure_selected == -K && flag_castle_00_black == 1
				&& from.y == 0 && from.x == 4 && to.y == 0 && to.x == 6
				&& board[0][5] == 0 && board[0][6] == 0 && board[0][7] == -R)
			{
				board[0][7] = 0;
				board[0][5] = -R;
				flag_castle_00_black = 0;
				rc = true;
			}
			else if(figure_selected == -K && flag_castle_000_black == 1
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
		else
		{
			rc = is_line_empty(from, to);
			if(rc)
			{
				if(figure_selected == K)
				{
					flag_castle_00_white = 0;
					flag_castle_000_white = 0;
				}
				if(figure_selected == -K)
				{
					flag_castle_00_black = 0;
					flag_castle_000_black = 0;
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

		if(figure_selected * flag_turn < 0
			|| from == to
			|| figure_selected * board[to.y][to.x] > 0)
			rc = false;
		else
		{
			switch(figure_selected)
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
		Vector2i from, to;
		std::string engine_move;

		engine_move = get_next_move(UCI.toAnsiString());

		from = get_xy_by_chess_coords(engine_move.substr(0, 2));
		to = get_xy_by_chess_coords(engine_move.substr(2, 2));

		UCI += get_chess_coords_by_xy(from);
		UCI += get_chess_coords_by_xy(to);
		UCI += " ";
		flag_turn = -flag_turn;

		board[to.y][to.x] = board[from.y][from.x];
		board[from.y][from.x] = 0;
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

	void run()
	{
		Vector2i mouse_xy, from, to;
		bool selected = false;

		Event event;

		while(window.isOpen())
		{
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
						std::cout << UCI.toAnsiString() << std::endl;
				}

				if(event.type == Event::MouseButtonPressed)
				{
					if(event.key.code == (Keyboard::Key)Mouse::Left)
					{
						selected = true;
						from = Mouse::getPosition(window);
						from.x /= CELL_SIZE;
						from.y /= CELL_SIZE;
						figure_selected = board[from.y][from.x];
						board[from.y][from.x] = 0;
					}
				}

				if(event.type == Event::MouseButtonReleased)
				{
					if(event.key.code == (Keyboard::Key)Mouse::Left)
					{
						selected = false;
						to = Mouse::getPosition(window);
						to.x /= CELL_SIZE;
						to.y /= CELL_SIZE;

						if(check_move(from, to))
						{
							board[to.y][to.x] = figure_selected;
							if (figure_selected != P && figure_selected != -P)
							{
								enpassant.y = 0;
								enpassant.x = 0;
							}
							UCI += get_chess_coords_by_xy(from);
							UCI += get_chess_coords_by_xy(to);
							UCI += " ";
							flag_turn = -flag_turn;

							make_engine_move();
						}
						else 
							board[from.y][from.x] = figure_selected;
					}
				}
			}
			window.clear();
			window.draw(s_board);
			draw_figures();

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

	// c.set_FEN("4Q3/2b4r/7B/6R1/5k/8/7K/5q2");
	// c.set_FEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
	// c.set_FEN("6k1/4Rpb1/6pp/1Np5/8/7P/2P2nPK/3r4 w - - 0 31");

	c.run();

	return 0;
}
