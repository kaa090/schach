// Возможности:
// + выгрузка FEN
// + загрузка FEN
// + редактор
// Доработать:
// - лог enpassant
// - рефакторинг переменных, функций(haskell way)
// - stockfish для linux

// Управление:
// Escape - выход
// BackSpace - отмена хода
// Space - сделать ход движком
// F - напечатать FEN
// U - напечатать UCI
// P - напечатать PGN
// M - напечатать myPGN
// G - напечатать рекомендуемый движком ход
// B - напечатать в текстовом виде ситуацию на доске
// E - включить/выключить движок
// C - режим редактирования позиции

#include <iostream>
#include <iomanip>
#include <SFML/Graphics.hpp>

#include <windows.h>
#include <stdio.h>
#include <string>
#include <sstream>

using namespace sf;

// startpos:
// #define defFEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

//Reti
// #define defFEN "7K/8/k1P5/7p/8/8/8/8 w - - 0 1"

//Filidor
// #define defFEN "4k3/8/8/4P3/3K4/8/R7/7r b - - 0 1"

// мат конём и слоном
// #define defFEN "4k3/8/8/8/8/8/8/4KBN1 w - - 99 1"

// смешная игра)
// #define defFEN "K7/PPPPPPPP/8/8/8/8/pppppppp/7k w - - 0 1"
// --------------------------------------------------------------------
// OK проверка: ход под шах
// #define defFEN "rn1qk2r/p1pp1pbp/bp2pnp1/8/8/BP2PNP1/P1PP1PBP/RN1QK2R w KQkq - 1 7"

// OK проверка: подсветка шаха чужому королю
// #define defFEN "k7/8/8/8/3K4/8/8/q7 w - - 0 1"

// OK проверка: мат/пат/50 ходов
// #define defFEN "k7/8/2K5/8/8/8/8/1Q6 w - - 1 1"

// проверка: ход связанной шахом фигурой
// #define defFEN "5k2/8/8/8/3q4/2N5/1K6/8 w - - 0 1"
// --------------------------------------------------------------------

const bool PLAY_WITH_ENGINE = 1;
const int BLACKSIDE = 1;
const int ENGINE_DEPTH = 8; // [1..20]
// jobava london system: [1..7]
// caro-kann:            [1..8]
// alien's gambit: ENGINE_DEPTH = 3

const int WAITTIME = 500;

//CHESS_SIZE = 0:
const int SQUARE_SIZE_0 = 50;
const char* PATH_BOARD_0 = "rsc/small_board.png";
const char* PATH_PIECES_0 = "rsc/small_pieces.png";

//CHESS_SIZE = 1:
const int SQUARE_SIZE_1 = 100;
const char* PATH_BOARD_1 = "rsc/big_board.png";
const char* PATH_PIECES_1 = "rsc/big_pieces.png";

//CHESS_SIZE = 7:
const int SQUARE_SIZE_7 = 50;
const char* PATH_BOARD_7 = "rsc/stealth_board.png";
const char* PATH_PIECES_7 = "rsc/stealth_pieces.png";

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

enum GameState
{
    InitScreen,
    MainGame,
    Promotion,
    GameOver
};

class Chess
{
    int square_size;
    RenderWindow window;
    Sprite s_board, s_piece;
    Texture t_board, t_pieces;
    Event event;
    int chess_size;
    int engine_depth;

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

    std::string UCI;
    std::string FEN;
    std::string PGN;
    std::string myPGN;

    int moves2mate = 0;
    int counter_move = 1;
    int counter_move50 = 0;
    int counter_halfmove = 0;
    int piece = 0;
    int piece_enemy = 0;
    int piece_selected = 0;
    int piece_promoted = Q;
    int move_side = MOVE_SIDE_WHITE;
    int castle_K, castle_Q, castle_k, castle_q;
    double eval = 0.0;
    Vector2i enpassant;
    Vector2i from, to;
    bool game_over = false;
    bool play_with_engine;
    bool single_side = false;
    bool modify_board = 0;
    bool engine_turn;
    bool selected;
    bool flag_need2check_mate = false;
    bool flag_check = false;
    bool flag_mate = false;
    bool flag_enpassant_captured = false;

    GameState state;

    PROCESS_INFORMATION proc_info;
    HANDLE pipin_w, pipin_r, pipout_w, pipout_r;

public:
    Chess()
    {
        init();
        init_scr(chess_size);

        proc_info = {0};
        connect_engine(PATH_ENGINE);        
    }

    ~Chess()
    {
        close_connection();
        FEN = get_FEN();
        std::cout << "PGN:" << std::endl << PGN << std::endl;
        std::cout << "FEN:" << std::endl << FEN << std::endl;
    }

    void clear_board()
    {
        for (int i = 0; i < 8; ++i)
            for (int j = 0; j < 8; ++j)
                board[i][j] = 0;
    }

    void init()
    {
        state = MainGame;
        chess_size = 1;
        engine_depth = ENGINE_DEPTH;
        counter_halfmove = 0;
        move_side = MOVE_SIDE_WHITE;
        castle_K = 1;
        castle_Q = 1;
        castle_k = 1;
        castle_q = 1;
        game_over = false;
        enpassant.x = 0;
        enpassant.y = 0;
        play_with_engine = PLAY_WITH_ENGINE;
        engine_turn = false;
        selected = false;
    }

    void init_scr(int _chess_size)
    {
        chess_size = _chess_size;

        if (chess_size == 0)
        {
            square_size = SQUARE_SIZE_0;
            t_board.loadFromFile(PATH_BOARD_0);
            t_pieces.loadFromFile(PATH_PIECES_0);
        }
        else if (chess_size == 1)
        {
            square_size = SQUARE_SIZE_1;
            t_board.loadFromFile(PATH_BOARD_1);
            t_pieces.loadFromFile(PATH_PIECES_1);
        }
        else if (chess_size == 7)
        {
            square_size = SQUARE_SIZE_7;
            t_board.loadFromFile(PATH_BOARD_7);
            t_pieces.loadFromFile(PATH_PIECES_7);
        }

        s_board.setTexture(t_board);
        s_piece.setTexture(t_pieces);
        window.create(VideoMode(8*square_size, 8*square_size), "kaaChess", Style::Close);
        window.setFramerateLimit(60);
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
            _FEN = _FEN + get_chess_coords_by_xy(enpassant);
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

    std::string get_chess_coords_by_xy(Vector2i xy)
    {
        std::string coords;

        coords += char(xy.x + 97);
        coords += char(56 - xy.y);

        return coords;
    }

    Vector2i get_xy_by_chess_coords(std::string coords)
    {
        Vector2i xy;

        xy.x = coords[0] - 97;
        xy.y = 56 - coords[1];

        return xy;
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

        return 0;
    }

    char get_piece_char(int piece)
    {
        switch (piece)
        {
            case P:     return 'P';
            case R:     return 'R';
            case N:     return 'N';
            case B:     return 'B';
            case Q:     return 'Q';
            case K:     return 'K';
            case -P:    return 'p';
            case -R:    return 'r';
            case -N:    return 'n';
            case -B:    return 'b';
            case -Q:    return 'q';
            case -K:    return 'k';
        }
    }

    void undo_move()
    {
        std::string last_move, sfrom, sto, castle_side;
        std::stringstream ss;
        Vector2i from, to;

        int piece = 0;
        int piece_enemy = 0;
        int piece_promo = 0;
        int find_capture, find_promo, find_castle, find_enpassant;

        ss.str(PGN);
        while (ss >> last_move){}
        last_move = " " + last_move;

        if (myPGN.length() == 0)
            return;

        PGN.erase(PGN.find(last_move), last_move.length());

        if (move_side == MOVE_SIDE_BLACK)
        {
            ss.clear();
            ss.str(PGN);
            while (ss >> last_move){}
            if (counter_move > 1)
                last_move = " " + last_move;
            PGN.erase(PGN.find(last_move), last_move.length());
        }

        ss.clear();
        ss.str(myPGN);
        while (ss >> last_move){}
        myPGN.erase(myPGN.find(last_move), last_move.length() + 1);

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

        size_t len = UCI.length();
        if ( len >= 5)
        {
            if (piece_promo != 0)
                UCI.erase(len - 6, 6);
            else
                UCI.erase(len - 5, 5);
        }

        if (game_over)
            game_over = false;

        move_side = -move_side;
        counter_halfmove--;

        if (counter_halfmove % 2 == 1)
        {
            counter_move--;            
        }

        set_counter_move50(piece, piece_enemy, -1);
    }

    void print_board(int _board[8][8])
    {
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 8; j++)
                std::cout << _board[i][j] << "\t";
            std::cout << std::endl;
        }
    }

    bool is_square_checked_horizontally(int _board[8][8], Vector2i square)
    {
        bool rc = false;

        for (int i = square.x - 1; i >= 0; i--)
        {
            if (_board[square.y][i] == 0)
                continue;

            if (_board[square.y][i] == -move_side * R || _board[square.y][i] == -move_side * Q)
                rc = true;

            break;
        }

        for (int i = square.x + 1; i <= 7; i++)
        {
            if (_board[square.y][i] == 0)
                continue;

            if (_board[square.y][i] == -move_side * R || _board[square.y][i] == -move_side * Q)
                rc = true;

            break;
        }

        return rc;
    }

    bool is_square_checked_vertically(int _board[8][8], Vector2i square)
    {
        bool rc = false;

        for (int i = square.y - 1; i >= 0; i--)
        {
            if (_board[i][square.x] == 0)
                continue;

            if (_board[i][square.x] == -move_side * R || _board[i][square.x] == -move_side * Q)
                rc = true;

            break;
        }

        for (int i = square.y + 1; i <= 7; i++)
        {
            if (_board[i][square.x] == 0)
                continue;

            if (_board[i][square.x] == -move_side * R || _board[i][square.x] == -move_side * Q)
                rc = true;

            break;
        }

        return rc;
    }

    bool is_square_checked_diagonally(int _board[8][8], Vector2i square)
    {
        bool rc = false;
        int i, j;

        for (i = square.y - 1, j = square.x - 1; i >= 0 && j >= 0; i--, j--)
        {
            if (_board[i][j] == 0)
                continue;

            if (_board[i][j] == -move_side * B || _board[i][j] == -move_side * Q)
                rc = true;

            break;
        }

        for (i = square.y - 1, j = square.x + 1; i >= 0 && j <= 7; i--, j++)
        {
            if (_board[i][j] == 0)
                continue;

            if (_board[i][j] == -move_side * B || _board[i][j] == -move_side * Q)
                rc = true;

            break;
        }

        for (i = square.y + 1, j = square.x - 1; i <= 7 && j >= 0; i++, j--)
        {
            if (_board[i][j] == 0)
                continue;

            if (_board[i][j] == -move_side * B || _board[i][j] == -move_side * Q)
                rc = true;

            break;
        }

        for (i = square.y + 1, j = square.x + 1; i <= 7 && j <= 7; i++, j++)
        {
            if (_board[i][j] == 0)
                continue;

            if (_board[i][j] == -move_side * B || _board[i][j] == -move_side * Q)
                rc = true;

            break;
        }

        return rc;
    }

    bool is_square_checked_by_knight(int _board[8][8], Vector2i square)
    {
        int x, y;

        y = square.y - 2;
        if (y >= 0)
        {
            x = square.x - 1;
            if (x >= 0)
                if (_board[y][x] == -move_side * N)
                    return true;

            x = square.x + 1;
            if (x <= 7)
                if (_board[y][x] == -move_side * N)
                    return true;
        }

        y = square.y - 1;
        if (y >= 0)
        {
            x = square.x - 2;
            if (x >= 0)
                if (_board[y][x] == -move_side * N)
                    return true;

            x = square.x + 2;
            if (x <= 7)
                if (_board[y][x] == -move_side * N)
                    return true;
        }

        y = square.y + 1;
        if (y <= 7)
        {
            x = square.x - 2;
            if (x >= 0)
                if (_board[y][x] == -move_side * N)
                    return true;

            x = square.x + 2;
            if (x <= 7)
                if (_board[y][x] == -move_side * N)
                    return true;
        }

        y = square.y + 2;
        if (y <= 7)
        {
            x = square.x - 1;
            if (x >= 0)
                if (_board[y][x] == -move_side * N)
                    return true;

            x = square.x + 1;
            if (x <= 7)
                if (_board[y][x] == -move_side * N)
                    return true;
        }

        return false;
    }

    bool is_square_checked_by_pawn(int _board[8][8], Vector2i square)
    {
        int x, y;

        x = square.x - 1;
        y = square.y - move_side;

        if (x >= 0 && y >= 0 && y <= 7)
            if (_board[y][x] == -move_side * P)
                return true;

        x = square.x + 1;

        if (x <= 7 && y >= 0 && y <= 7)
            if (_board[y][x] == -move_side * P)
                return true;

        return false;
    }

    Vector2i is_square_checked_by_my_rook(Vector2i from, Vector2i to)
    {
        Vector2i square_rook;
        square_rook.x = -1;
        square_rook.y = -1;

        int x, y;
        int rook;

        rook = move_side * R;

        if (from.x > to.x)
        {
            for (int i = to.x - 1; i >= 0; i--)
            {
                if (board[to.y][i] == 0)
                    continue;

                if (board[to.y][i] == rook)
                {
                    square_rook.x = i;
                    square_rook.y = to.y;
                    return square_rook;
                }

                break;
            }
        }
        else
        {
            for (int i = to.x + 1; i <= 7; i++)
            {
                if (board[to.y][i] == 0)
                    continue;

                if (board[to.y][i] == rook)
                {
                    square_rook.x = i;
                    square_rook.y = to.y;
                    return square_rook;
                }

                break;
            }
        }

        if (from.y > to.y)
        {
            for (int i = to.y - 1; i >= 0; i--)
            {
                if (board[i][to.x] == 0)
                    continue;

                if (board[i][to.x] == rook)
                {
                    square_rook.x = to.x;
                    square_rook.y = i;
                    return square_rook;
                }

                break;
            }
        }
        else
        {
            for (int i = to.y + 1; i <= 7; i++)
            {
                if (board[i][to.x] == 0)
                    continue;

                if (board[i][to.x] == rook)
                {
                    square_rook.x = to.x;
                    square_rook.y = i;
                    return square_rook;
                }

                break;
            }
        }


        return square_rook;
    }

    Vector2i is_square_checked_by_my_knight(Vector2i to)
    {
        Vector2i square_knight;
        square_knight.x = -1;
        square_knight.y = -1;

        int x, y;
        int knight;

        knight = move_side * N;

        y = to.y - 2;
        if (y >= 0)
        {
            x = to.x - 1;
            if (x >= 0)
            {
                if (board[y][x] == knight)
                {
                    square_knight.x = x;
                    square_knight.y = y;
                    return square_knight;
                }
            }

            x = to.x + 1;
            if (x <= 7)
            {
                if (board[y][x] == knight)
                {
                    square_knight.x = x;
                    square_knight.y = y;
                    return square_knight;
                }
            }
        }

        y = to.y - 1;
        if (y >= 0)
        {
            x = to.x - 2;
            if (x >= 0)
            {
                if (board[y][x] == knight)
                {
                    square_knight.x = x;
                    square_knight.y = y;
                    return square_knight;
                }
            }

            x = to.x + 2;
            if (x <= 7)
            {
                if (board[y][x] == knight)
                {
                    square_knight.x = x;
                    square_knight.y = y;
                    return square_knight;
                }
            }
        }

        y = to.y + 1;
        if (y <= 7)
        {
            x = to.x - 2;
            if (x >= 0)
            {
                if (board[y][x] == knight)
                {
                    square_knight.x = x;
                    square_knight.y = y;
                    return square_knight;
                }
            }

            x = to.x + 2;
            if (x <= 7)
            {
                if (board[y][x] == knight)
                {
                    square_knight.x = x;
                    square_knight.y = y;
                    return square_knight;
                }
            }
        }

        y = to.y + 2;
        if (y <= 7)
        {
            x = to.x - 1;
            if (x >= 0)
            {
                if (board[y][x] == knight)
                {
                    square_knight.x = x;
                    square_knight.y = y;
                    return square_knight;
                }
            }

            x = to.x + 1;
            if (x <= 7)
            {
                if (board[y][x] == knight)
                {
                    square_knight.x = x;
                    square_knight.y = y;
                    return square_knight;
                }
            }
        }

        return square_knight;
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
        bool rc;
        int y_captured;

        if (piece_selected == P  && to.x == enpassant.x && to.y == enpassant.y && from.y == 3
            || 
            piece_selected == -P && to.x == enpassant.x && to.y == enpassant.y && from.y == 4)
        {
            if (piece_selected == P)
                y_captured = enpassant.y + 1;
            else if (piece_selected == -P)
                y_captured = enpassant.y - 1;

            board[y_captured][enpassant.x] = 0;
            flag_enpassant_captured = true;
            rc = true;
        }
        else
            rc = false;

        return rc;
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

    bool check_square_checked(int _board[8][8], Vector2i square)
    {
        bool rc = false;

        rc |= is_square_checked_horizontally(_board, square);
        rc |= is_square_checked_vertically(_board, square);
        rc |= is_square_checked_diagonally(_board, square);
        rc |= is_square_checked_by_knight(_board, square);
        rc |= is_square_checked_by_pawn(_board, square);
        return rc;
    }

    bool check_move_king(Vector2i from, Vector2i to)
    {
        bool rc = true;
        int board_curr[8][8];
        int board_next[8][8];
        Vector2i square_king;

        if (abs(from.x - to.x) > 1)
        {
            for (int i = 0; i < 8; i++)
                for (int j = 0; j < 8; j++)
                    board_curr[i][j] = board[i][j];

            board_curr[from.y][from.x] = piece_selected;
            square_king = find_king(board_curr);

            if (check_square_checked(board_curr, square_king))
            {
                rc = false;
            }
            else
            {
                if (piece_selected == K && castle_K == 1
                    && from.y == 7 && from.x == 4 && to.y == 7 && to.x == 6
                    && board[7][5] == 0 && board[7][6] == 0 && board[7][7] == R
                    && !check_square_checked(board, Vector2i(5,7))
                    && !check_square_checked(board, Vector2i(6,7)))
                {
                    board[7][7] = 0;
                    board[7][5] = R;
                    castle_K = 0;
                    castle_Q = 0;
                }
                else if (piece_selected == K && castle_Q == 1
                    && from.y == 7 && from.x == 4 && to.y == 7 && to.x == 2
                    && board[7][3] == 0 && board[7][2] == 0 && board[7][1] == 0 && board[7][0] == R
                    && !check_square_checked(board, Vector2i(2,7))
                    && !check_square_checked(board, Vector2i(3,7)))
                {
                    board[7][0] = 0;
                    board[7][3] = R;
                    castle_K = 0;
                    castle_Q = 0;
                }
                else if (piece_selected == -K && castle_k == 1
                    && from.y == 0 && from.x == 4 && to.y == 0 && to.x == 6
                    && board[0][5] == 0 && board[0][6] == 0 && board[0][7] == -R
                    && !check_square_checked(board, Vector2i(5,0))
                    && !check_square_checked(board, Vector2i(6,0)))
                {
                    board[0][7] = 0;
                    board[0][5] = -R;
                    castle_k = 0;
                    castle_q = 0;
                }
                else if (piece_selected == -K && castle_q == 1
                    && from.y == 0 && from.x == 4 && to.y == 0 && to.x == 2
                    && board[0][3] == 0 && board[0][2] == 0 && board[0][1] == 0 && board[0][0] == -R
                    && !check_square_checked(board, Vector2i(2,0))
                    && !check_square_checked(board, Vector2i(3,0)))
                {
                    board[0][0] = 0;
                    board[0][3] = -R;
                    castle_k = 0;
                    castle_q = 0;
                }
                else
                    rc = false;                
            }
        }
        else if (abs(from.y - to.y) <= 1 && abs(from.x - to.x) <= 1)
        {
            rc = is_line_empty(from, to);
            if (rc)
            {
                for (int i = 0; i < 8; i++)
                    for (int j = 0; j < 8; j++)
                        board_next[i][j] = board[i][j];

                board_next[to.y][to.x] = piece_selected;
                square_king = find_king(board_next);

                if (check_square_checked(board_next, square_king))
                    rc = false;

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
                {
                    piece_selected = piece_promoted;
                }

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
                {
                    piece_selected = piece_promoted;
                }

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
        Vector2i square_king;

        int board_curr[8][8];
        int board_next[8][8];

        if (modify_board)
            return true;

        for (int i = 0; i < 8; i++)
            for (int j = 0; j < 8; j++)
            {
                board_curr[i][j] = board[i][j];
                board_next[i][j] = board[i][j];
            }

        board_curr[from.y][from.x] = piece_selected;
        board_next[to.y][to.x] = piece_selected;

        square_king = find_king(board_curr);

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
        
        square_king = find_king(board_next);
        if (check_square_checked(board_next, square_king))
            rc = false;

        return rc;
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

            if (flag_check)
                flag_mate = true;

            return;
        }

        if (counter_move50 >= 100)
            return;

        from = get_xy_by_chess_coords(engine_move.substr(0, 2));
        to = get_xy_by_chess_coords(engine_move.substr(2, 2));
        piece_promoted = get_piece_code(engine_move[4]);

        if (to.y == 0)
            piece_promoted = -piece_promoted;

        piece_selected = board[from.y][from.x];
        piece_enemy = board[to.y][to.x];

        piece_selected_old = piece_selected;
        check_move(from, to);

        board[to.y][to.x] = piece_selected;
        board[from.y][from.x] = 0;

        log_game(piece_selected_old, from, to, piece_enemy);

        counter_halfmove++;

        if (move_side == MOVE_SIDE_BLACK)
        {
            counter_move++;
        }

        set_counter_move50(piece_selected_old, piece_enemy, 1);

        move_side = -move_side;

        if (moves2mate == 1 && engine_turn)
        {
            game_over = true;
            flag_mate = true;
        }
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
        int n, n_nodes;
        BYTE buffer[2048];
        DWORD bytes, bytes_available;
        std::string UCI_command;
        std::string UCI_answer;
        std::string str_find;
        std::string str_find_move = "bestmove";
        std::string str_find_mate = "score mate";
        std::string str_find_nodes = "nodes";
        std::string str_find_score = "score cp";

        if (FEN.length())
        {
            if (UCI.length() == 0)
            {
                UCI_command = "position fen " + FEN + "\ngo depth " + std::to_string(engine_depth) + "\n";
            }
            else
            {
                UCI_command = "position fen " + FEN + " moves " + UCI + "\ngo depth " + std::to_string(engine_depth) + "\n";
            }
        }
        else
        {
            UCI_command = "position startpos moves " + UCI + "\ngo depth " + std::to_string(engine_depth) + "\n";
        }

        WriteFile(pipin_w, UCI_command.c_str(), UCI_command.length(), &bytes, NULL);
        Sleep(WAITTIME);

        PeekNamedPipe(pipout_r, buffer, sizeof(buffer), &bytes, &bytes_available, NULL);
        do
        {
            ZeroMemory(buffer, sizeof(buffer));
            if (!ReadFile(pipout_r, buffer, sizeof(buffer), &bytes, NULL) || !bytes) break;
            buffer[bytes] = 0;
            UCI_answer += (char*)buffer;
        }
        while (bytes >= sizeof(buffer));

        n = UCI_answer.find(str_find_mate);


        if(n > 0)
        {
            n_nodes = UCI_answer.find(str_find_nodes, n);
            moves2mate = stod(UCI_answer.substr(n + str_find_mate.length() + 1, n_nodes - (n + str_find_mate.length() + 1)));
        }
        else
        {
            n = UCI_answer.find(str_find_score);
            if(n > 0)
                eval = stod(UCI_answer.substr(n + str_find_score.length() + 1, 5)) / 100 * move_side;
        }

        n = UCI_answer.find(str_find_move);

        if (n != -1)
            return UCI_answer.substr(n + str_find_move.length() + 1, 5);
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

    void set_title()
    {
        std::stringstream ss;

        ss << counter_move << ". ";

        if (counter_halfmove % 2 == 0)
            ss << "White move ( ";
        else
            ss << "Black move ( ";

        if(moves2mate != 0)
            ss << "mate in " << abs(moves2mate) << " moves";
        else
            ss << std::fixed << std::setprecision(2) << eval;

         ss << " )";

        if (!single_side)
            window.setTitle(ss.str());
    }

    void log_PGN(int piece, Vector2i from, Vector2i to, int piece_enemy)
    {
        int piece_ABS;
        std::string sfrom, sto;
        Vector2i square_knight;
        Vector2i square_rook;

        sfrom = get_chess_coords_by_xy(from);
        sto = get_chess_coords_by_xy(to);
        piece_ABS = abs(piece);

        if (PGN != "")
            PGN += " ";

        if (move_side == MOVE_SIDE_WHITE)
            PGN += std::to_string(counter_move) + ". ";
        else if(move_side == MOVE_SIDE_BLACK && PGN.length() == 0)
            PGN += std::to_string(counter_move) + ". ... ";

        if (piece == K && sfrom == "e1" && sto == "g1"
         || piece == -K && sfrom == "e8" && sto == "g8")
        {
            PGN += "0-0";
        }
        else if (piece == K && sfrom == "e1" && sto == "c1"
              || piece == -K && sfrom == "e8" && sto == "c8")
        {
            PGN += "0-0-0";
        }
        else
        {
            if (piece_ABS != P)
                PGN += get_piece_char(piece_ABS);
                if (piece_ABS == N)
                {
                    square_knight = is_square_checked_by_my_knight(to);

                    if (square_knight.x != -1 && square_knight.y != -1)
                    {
                        if (from.x != square_knight.x)
                            PGN += get_chess_coords_by_xy(from)[0];
                        else
                            PGN += get_chess_coords_by_xy(from)[1];
                    }
                }
                else if (piece_ABS == R)
                {
                    square_rook = is_square_checked_by_my_rook(from, to);

                    if (square_rook.x != -1 && square_rook.y != -1)
                    {
                        if (from.x != square_rook.x)
                            PGN += get_chess_coords_by_xy(from)[0];
                        else
                            PGN += get_chess_coords_by_xy(from)[1];
                    }
                }

            if (piece_enemy != 0)
            {
                if (piece_ABS == P)
                    PGN += sfrom[0];

                PGN += "x";
            }

            PGN += sto;

            if (piece == P && sto[1] == '8')
                PGN += "=Q";
            else if (piece == -P && sto[1] == '1')
                PGN += "=q";
        }
    }

    void set_counter_move50(int piece, int piece_enemy, int direction)
    {
        if (abs(piece) == P || piece_enemy != 0)
            counter_move50 = 0;
        else
            counter_move50 += direction;
    }

    void log_game(int piece, Vector2i from, Vector2i to, int piece_enemy)
    {
        std::string sfrom, sto;

        sfrom = get_chess_coords_by_xy(from);
        sto = get_chess_coords_by_xy(to);

        UCI += sfrom;
        UCI += sto;

        log_PGN(piece, from, to, piece_enemy);

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
            if (piece_promoted == Q)
                { myPGN += "=Q"; UCI += "q"; }
            else if (piece_promoted == R)
                { myPGN += "=R"; UCI += "r"; }
            else if (piece_promoted == N)
                { myPGN += "=N"; UCI += "n"; }
            else if (piece_promoted == B)
                { myPGN += "=B"; UCI += "b"; }
        }
        else if (piece == -P && sto[1] == '1')
        {
            if (piece_promoted == -Q)
                { myPGN += "=q"; UCI += "q"; }
            else if (piece_promoted == -R)
                { myPGN += "=r"; UCI += "r"; }
            else if (piece_promoted == -N)
                { myPGN += "=n"; UCI += "n"; }
            else if (piece_promoted == -B)
                { myPGN += "=b"; UCI += "b"; }
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

    bool is_promotion()
    {
        if (piece_selected == P && from.y == 1 && to.y == 0
             ||
            piece_selected == -P && from.y == 6 && to.y == 7)
            return true;
        else
            return false;
    }

    void make_move()
    {
        if (check_move(from, to))
        {
            piece_enemy = board[to.y][to.x];
            board[to.y][to.x] = piece_selected;

            if (modify_board == false)
            {
                log_game(piece, from, to, piece_enemy);

                counter_halfmove++;

                if (move_side == MOVE_SIDE_BLACK)
                    counter_move++;

                set_counter_move50(piece, piece_enemy, 1);

                if (!single_side)
                    move_side = -move_side;

                if (play_with_engine)
                    engine_turn = true;
            }

            if (piece_selected != P && piece_selected != -P || flag_enpassant_captured)
            {
                enpassant.y = 0;
                enpassant.x = 0;
                flag_enpassant_captured = false;
            }
        }
        else
        {
            board[from.y][from.x] = piece_selected;
        }
    }

    void check_mate()
    {
        std::string engine_move;

        if (!game_over)
        {
            engine_move = get_engine_move();
            if (engine_move == "(none")
            {
                game_over = true;

                if (flag_check == true)
                    flag_mate = true;
            }
        }        
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

                    if (game_over && (board[i][j] * move_side == K))
                        piece_xy.x = 6;

                    if (flag_mate == false && game_over && (board[i][j] * move_side == -K))
                        piece_xy.x = 6;

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

    void draw_pieces_promo()
    {
        if (BLACKSIDE)
        {
            if (move_side == MOVE_SIDE_BLACK)
            {
                s_piece.setTextureRect(IntRect(square_size, 1*square_size, 4*square_size, 2*square_size));
                s_piece.setPosition(2*square_size, 1*square_size);
            }
            else
            {
                s_piece.setTextureRect(IntRect(square_size, 0*square_size, 4*square_size, 1*square_size));
                s_piece.setPosition(2*square_size, 6*square_size);
            }
        }
        else
        {
            if (move_side == MOVE_SIDE_WHITE)
            {
                s_piece.setTextureRect(IntRect(square_size, 0*square_size, 4*square_size, 1*square_size));
                s_piece.setPosition(2*square_size, 1*square_size);
            }
            else
            {
                s_piece.setTextureRect(IntRect(square_size, 1*square_size, 4*square_size, 2*square_size));
                s_piece.setPosition(2*square_size, 6*square_size);
            }
        }

        window.draw(s_piece);
    }

    void draw_board()
    {

        window.draw(s_board);

        if (state == MainGame)
        {
            draw_lastmove();
            draw_check();
        }
    }

    void draw_lastmove()
    {
        std::stringstream ss;
        std::string last_move;
        Vector2i from, to;

        RectangleShape rect;

        ss.str(UCI);
        while (ss >> last_move){}

        if (last_move.length() > 0)
        {
            from = get_xy_by_chess_coords(last_move.substr(0, 2));
            to   = get_xy_by_chess_coords(last_move.substr(2, 2));

            if (BLACKSIDE)
            {
                from.x = 7 - from.x;
                from.y = 7 - from.y;
                to.x = 7 - to.x;
                to.y = 7 - to.y;
            }

            rect.setSize(Vector2f(square_size, square_size));
            rect.setFillColor(Color(255,255,0,50));

            rect.setPosition(from.x*square_size,from.y*square_size);
            window.draw(rect);

            rect.setPosition(to.x*square_size,to.y*square_size);
            window.draw(rect);
        }
    }

    void draw_check()
    {
        RectangleShape rect;
        Vector2i square;
        Vector2i square_king;
        int board_curr[8][8];

        for (int i = 0; i < 8; i++)
            for (int j = 0; j < 8; j++)
                board_curr[i][j] = board[i][j];
        
        if (selected && piece_selected == move_side * K)
        {
            board_curr[from.y][from.x] = piece_selected;
        }

        square_king = find_king(board_curr);        
        flag_check = check_square_checked(board_curr, square_king);

        if (flag_check)
        {
            if (BLACKSIDE)
            {
                square_king.x = 7 - square_king.x;
                square_king.y = 7 - square_king.y;
            }

            rect.setSize(Vector2f(square_size, square_size));
            rect.setFillColor(Color(255,0,0,50));

            rect.setPosition(square_king.x*square_size,square_king.y*square_size);
            window.draw(rect);
        }
    }

    void handleEvents()
    {
        while (window.pollEvent(event))
        {
            switch(state)
            {
                case MainGame:
                    handleEvents_main();
                    break;

                case Promotion:
                    handleEvents_promo();
                    break;
            }
        }
    }

    void handleEvents_promo()
    {
        Vector2i click;

        if (event.type == Event::Closed)
        {
            close_connection();
            window.close();
        }

        if (event.type == Event::KeyPressed)
        {
            if (event.key.code == Keyboard::Escape)
            {
                close_connection();
                window.close();
            }
        }

        if (event.type == Event::MouseButtonPressed)
        {
            if (event.key.code == (Keyboard::Key)Mouse::Left)
            {
            }
        }

        if (event.type == Event::MouseButtonReleased)
        {
            if (event.key.code == (Keyboard::Key)Mouse::Left)
            {
                click = Mouse::getPosition(window);
                click.x /= square_size;
                click.y /= square_size;

                if (click.x == 2)
                    piece_promoted = Q;
                else if (click.x == 3)
                    piece_promoted = B;
                else if (click.x == 4)
                    piece_promoted = N;
                else if (click.x == 5)
                    piece_promoted = R;

                if (move_side == MOVE_SIDE_BLACK)
                {
                    piece_promoted = -piece_promoted;
                }

                if (BLACKSIDE)
                {
                    click.x = 7 - click.x;
                    click.y = 7 - click.y;
                }

                make_move();
                state = MainGame;
            }
        }
    }

    void handleEvents_main()
    {
        if (event.type == Event::Closed)
        {
            close_connection();
            window.close();
        }

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

            if (event.key.code == Keyboard::B)
                print_board(board);

            if (event.key.code == Keyboard::C)
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

            if (event.key.code == Keyboard::E)
            {
                play_with_engine = !play_with_engine;
                if (play_with_engine)
                {
                    engine_turn = true;
                    std::cout << "Движок включён" << std::endl;
                }
                else
                    std::cout << "Движок выключён" << std::endl;
            }

            if (event.key.code == Keyboard::F)
                std::cout << "FEN:" << std::endl << get_FEN() << std::endl;

            if (event.key.code == Keyboard::P)
                std::cout << "PGN:" << std::endl << PGN << std::endl;

            if (event.key.code == Keyboard::M)
                std::cout << "myPGN:" << std::endl << myPGN << std::endl;

            if (event.key.code == Keyboard::U)
                std::cout << "UCI:" << std::endl << UCI << std::endl;

            if (event.key.code == Keyboard::Up)
            {
                engine_depth++;
                if (engine_depth >= 17)
                {
                    engine_depth = 17;
                }
                std::cout<<"engine_depth = "<<engine_depth<<std::endl;
            }

            if (event.key.code == Keyboard::Down)
            {
                engine_depth--;
                if (engine_depth <= 1)
                {
                    engine_depth = 1;
                }
                std::cout<<"engine_depth = "<<engine_depth<<std::endl;
            }

            if (event.key.code == Keyboard::Num1)
            {
                init_scr(0);
            }

            if (event.key.code == Keyboard::Num2)
            {
                init_scr(1);
            }

            if (event.key.code == Keyboard::Tilde)
            {
                init_scr(7);
            }

            if (event.key.code == Keyboard::S)
            {
                single_side = !single_side;
                if (single_side)
                {
                    std::cout << "Режим одной стороны" << std::endl;
                    move_side = MOVE_SIDE_WHITE;
                }
                else
                    std::cout << "Режим двух сторон" << std::endl;
            }
        }

        if (event.type == Event::MouseButtonPressed && game_over == false)
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

        if (event.type == Event::MouseButtonReleased && selected == true && game_over == false)
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

                if (is_promotion())
                    state = Promotion;
                else
                    make_move();

                flag_need2check_mate = true;
            }
        }
    }

    void updateGame()
    {
        Vector2i square_king;

        if (engine_turn && game_over == false)
        {
            make_engine_move();
            engine_turn = false;

            if (moves2mate != 1)
            {
                flag_need2check_mate = true;
            }
        }

        if (piece_selected == move_side * K)
            square_king = from;
        else
            square_king = find_king(board);

        if (check_square_checked(board, square_king) && flag_need2check_mate && play_with_engine == false)
        {
            check_mate();
            flag_need2check_mate = false;
        }            

        if (counter_move50 >= 100)
        {
            game_over = true;
        }
    }

    void drawGame()
    {
        window.clear();

        switch(state)
        {
            case InitScreen:
                break;

            case MainGame:
                draw_board();
                draw_pieces();

                if (selected)
                    draw_move();

                break;

            case Promotion:
                draw_board();
                draw_pieces_promo();

                break;

            case GameOver:
                break;
        }

        set_title();
        window.display();
    }

    int run()
    {
        if (PLAY_WITH_ENGINE == 1)
        {
            if (BLACKSIDE == 1 && move_side == MOVE_SIDE_WHITE
                || BLACKSIDE == 0 && move_side == MOVE_SIDE_BLACK)
                engine_turn = true;
        }

        while(window.isOpen())
        {
            handleEvents();
            drawGame();
            updateGame();
        }

        return 0;
    }
};

int main(int argc, char const *argv[])
{
    
    Chess c;

    #ifdef defFEN
        c.set_FEN(defFEN);
    #endif

    return c.run();
}