#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cctype>
#include <string>

using namespace std;

const int BOARD_SIZE = 8;

enum Cell { EMPTY, WHITE, BLACK, WHITE_KING, BLACK_KING };

struct Position {
    int row, col;
    Position(int r = 0, int c = 0) : row(r), col(c) {}
};

struct Piece {
    Cell type;
    bool isKing;
    Piece(Cell t = EMPTY, bool king = false) : type(t), isKing(king) {}
};

Piece board[BOARD_SIZE][BOARD_SIZE];
mutex board_mutex;

void initialize_board() {
    for(int i = 0; i < BOARD_SIZE; ++i)
        for(int j = 0; j < BOARD_SIZE; ++j)
            board[i][j] = Piece();

    for(int row = 0; row < 3; ++row) {
        for(int col = (row % 2 == 0 ? 1 : 0); col < BOARD_SIZE; col +=2) {
            board[row][col] = Piece(WHITE, false);
        }
    }

    for(int row = 5; row < BOARD_SIZE; ++row) {
        for(int col = (row % 2 == 0 ? 1 : 0); col < BOARD_SIZE; col +=2) {
            board[row][col] = Piece(BLACK, false);
        }
    }
}

void display_board() {
    cout << "  A B C D E F G H\n";
    for(int row = BOARD_SIZE-1; row >=0; --row) {
        cout << row+1 << " ";
        for(int col = 0; col < BOARD_SIZE; ++col) {
            switch(board[row][col].type) {
                case EMPTY: cout << ". "; break;
                case WHITE: cout << "w "; break;
                case BLACK: cout << "b "; break;
                case WHITE_KING: cout << "W "; break;
                case BLACK_KING: cout << "B "; break;
            }
        }
        cout << endl;
    }
}

bool is_valid_position(int row, int col) {
    return row >=0 && row < BOARD_SIZE && col >=0 && col < BOARD_SIZE;
}

bool make_move(Position start, Position end, Cell player) {
    lock_guard<mutex> lock(board_mutex);

    if(!is_valid_position(start.row, start.col) || !is_valid_position(end.row, end.col))
        return false;

    Piece moving_piece = board[start.row][start.col];
    if(moving_piece.type != player && moving_piece.type != (player == WHITE ? WHITE_KING : BLACK_KING))
        return false;
    if(board[end.row][end.col].type != EMPTY)
        return false;

    int d_row = end.row - start.row;
    int d_col = end.col - start.col;

    bool is_capture = false;
    if(abs(d_row) == 2 && abs(d_col) == 2) {
        int mid_row = start.row + d_row / 2;
        int mid_col = start.col + d_col / 2;
        Cell opponent = (player == WHITE || player == WHITE_KING) ? BLACK : WHITE;

        if(board[mid_row][mid_col].type == opponent ||
           board[mid_row][mid_col].type == (opponent == WHITE ? BLACK_KING : WHITE_KING)) {
            board[mid_row][mid_col] = Piece();
            is_capture = true;
        } else {
            return false;
        }
    }
    else if(abs(d_row) !=1 || abs(d_col)!=1) {
        return false;
    }

    if(!moving_piece.isKing) {
        if(player == WHITE && d_row <=0)
            return false;
        if(player == BLACK && d_row >=0)
            return false;
    }

    board[end.row][end.col] = moving_piece;
    board[start.row][start.col] = Piece();

    if((player == WHITE || player == WHITE_KING) && end.row == BOARD_SIZE-1 && !moving_piece.isKing) {
        board[end.row][end.col].isKing = true;
        board[end.row][end.col].type = WHITE_KING;
    }
    if((player == BLACK || player == BLACK_KING) && end.row ==0 && !moving_piece.isKing) {
        board[end.row][end.col].isKing = true;
        board[end.row][end.col].type = BLACK_KING;
    }

    return true;
}

bool parse_move(const string& input, Position& start, Position& end) {
    if(input.length() != 5 || input[2] != ' ') return false;

    char start_col = toupper(input[0]);
    char start_row = input[1];
    char end_col = toupper(input[3]);
    char end_row = input[4];

    if(start_col < 'A' || start_col > 'H' || end_col < 'A' || end_col > 'H')
        return false;
    if(start_row < '1' || start_row > '8' || end_row < '1' || end_row > '8')
        return false;

    start.col = start_col - 'A';
    start.row = start_row - '1';
    end.col = end_col - 'A';
    end.row = end_row - '1';

    return true;
}

vector<pair<Position, Position>> get_all_possible_moves(Cell player) {
    vector<pair<Position, Position>> moves;

    for(int row =0; row < BOARD_SIZE; row++) {
        for(int col=0; col < BOARD_SIZE; col++) {
            Cell current = board[row][col].type;
            if(current != player && current != (player == WHITE ? WHITE_KING : BLACK_KING))
                continue;

            vector<int> directions;
            if(current == WHITE || current == WHITE_KING) {
                directions.push_back(1);
            }
            if(current == BLACK || current == BLACK_KING) {
                directions.push_back(-1);
            }
            if(board[row][col].isKing) {
                directions.push_back(-1);
                directions.push_back(1);
            }

            for(auto dir : directions) {
                for(int d_col = -1; d_col <=1; d_col +=2) {
                    int new_row = row + dir;
                    int new_col = col + d_col;
                    if(is_valid_position(new_row, new_col) && board[new_row][new_col].type == EMPTY) {
                        moves.emplace_back(make_pair(Position(row, col), Position(new_row, new_col)));
                    }
                }

                for(int d_col = -1; d_col <=1; d_col +=2) {
                    int mid_row = row + dir;
                    int mid_col = col + d_col;
                    int end_row = row + 2*dir;
                    int end_col = col + 2*d_col;

                    if(is_valid_position(end_row, end_col) && board[end_row][end_col].type == EMPTY) {
                        Cell opponent = (player == WHITE || player == WHITE_KING) ? BLACK : WHITE;
                        if(board[mid_row][mid_col].type == opponent ||
                           board[mid_row][mid_col].type == (opponent == WHITE ? BLACK_KING : WHITE_KING)) {
                            moves.emplace_back(make_pair(Position(row, col), Position(end_row, end_col)));
                        }
                    }
                }
            }
        }
    }

    bool has_capture = false;
    for(auto& move : moves)
        if(abs(move.second.row - move.first.row) == 2)
            { has_capture = true; break; }

    if(has_capture) {
        vector<pair<Position, Position>> capture_moves;
        for(auto& move : moves)
            if(abs(move.second.row - move.first.row) == 2)
                capture_moves.emplace_back(move);
        return capture_moves;
    }

    return moves;
}

void display_possible_moves(Cell player) {
    vector<pair<Position, Position>> possible_moves = get_all_possible_moves(player);
    if(possible_moves.empty()) {
        cout << "Нет доступных ходов.\n";
        return;
    }
    cout << "Возможные ходы:\n";
    for(auto& move : possible_moves) {
        char start_col = 'A' + move.first.col;
        char start_row = '1' + move.first.row;
        char end_col = 'A' + move.second.col;
        char end_row = '1' + move.second.row;
        cout << start_col << start_row << " " << end_col << end_row << "\n";
    }
}

void player_turn(Cell player) {
    while(true) {
        display_possible_moves(player);
        cout << "Введите ваш ход (например, A3 B4): ";
        string input;
        getline(cin, input);
        if(input.empty()) { getline(cin, input); }

        Position start, end;
        if(!parse_move(input, start, end)) {
            cout << "Некорректный ввод. Попробуйте снова.\n";
            continue;
        }

        vector<pair<Position, Position>> possible_moves = get_all_possible_moves(player);
        bool valid = false;
        for(auto& move : possible_moves)
            if(move.first.row == start.row && move.first.col == start.col &&
               move.second.row == end.row && move.second.col == end.col) {
                   valid = true;
                   break;
               }

        if(valid) {
            if(make_move(start, end, player)) {
                break;
            } else {
                cout << "Не удалось выполнить ход. Попробуйте снова.\n";
            }
        } else {
            cout << "Недопустимый ход. Попробуйте снова.\n";
        }
    }
}

struct MoveEvaluation {
    pair<Position, Position> move;
    int score;
};

int evaluate_move(pair<Position, Position> move, Cell player) {
    int score = 0;
    Position start = move.first;
    Position end = move.second;

    if(abs(end.row - start.row) ==2) {
        score += 10;
    }

    if(player == WHITE && end.row == BOARD_SIZE-1)
        score += 5;
    if(player == BLACK && end.row ==0)
        score +=5;

    if((player == WHITE && board[start.row][start.col].type == WHITE && end.row == BOARD_SIZE-1) ||
       (player == BLACK && board[start.row][start.col].type == BLACK && end.row ==0)) {
        score +=5;
    }

    return score;
}

void ai_turn(Cell player) {
    vector<pair<Position, Position>> possible_moves = get_all_possible_moves(player);
    if(possible_moves.empty()) return;

    vector<MoveEvaluation> evaluations(possible_moves.size());
    vector<thread> threads;
    int num_moves = possible_moves.size();

    auto evaluate = [&](int idx) {
        evaluations[idx].move = possible_moves[idx];
        evaluations[idx].score = evaluate_move(possible_moves[idx], player);
    };

    for(int i=0; i < num_moves; ++i) {
        threads.emplace_back(evaluate, i);
    }

    for(auto& th : threads) {
        th.join();
    }

    int best_score = -1000;
    int best_move_idx = 0;
    for(int i=0; i < evaluations.size(); ++i) {
        if(evaluations[i].score > best_score) {
            best_score = evaluations[i].score;
            best_move_idx = i;
        }
    }

    make_move(evaluations[best_move_idx].move.first, evaluations[best_move_idx].move.second, player);
    Position start = evaluations[best_move_idx].move.first;
    Position end = evaluations[best_move_idx].move.second;
    char start_col = 'A' + start.col;
    char start_row = '1' + start.row;
    char end_col = 'A' + end.col;
    char end_row = '1' + end.row;
    cout << "ИИ делает ход: " << start_col << start_row << " " << end_col << end_row << "\n";
}

bool check_game_over(Cell player, Cell ai_player) {
    bool player_has_pieces = false, ai_has_pieces = false;
    for(int i =0; i < BOARD_SIZE; i++)
        for(int j=0; j < BOARD_SIZE; j++) {
            if(board[i][j].type == player || board[i][j].type == (player == WHITE ? WHITE_KING : BLACK_KING))
                player_has_pieces = true;
            if(board[i][j].type == ai_player || board[i][j].type == (ai_player == WHITE ? WHITE_KING : BLACK_KING))
                ai_has_pieces = true;
        }

    if(!player_has_pieces || !ai_has_pieces)
        return true;

    vector<pair<Position, Position>> player_moves = get_all_possible_moves(player);
    vector<pair<Position, Position>> ai_moves = get_all_possible_moves(ai_player);
    if(player_moves.empty() || ai_moves.empty())
        return true;

    return false;
}

int main() {
    initialize_board();
    char choice;
    cout << "Выберите цвет: белые (w) или черные (b): ";
    cin >> choice;
    cin.ignore();

    Cell player = (tolower(choice) == 'w') ? WHITE : BLACK;
    Cell ai_player = (player == WHITE) ? BLACK : WHITE;

    bool player_turn_flag = (player == WHITE) ? true : false;

    while(true) {
        display_board();

        if(check_game_over(player, ai_player)) {
            bool player_has_pieces = false, ai_has_pieces = false;
            for(int i =0; i < BOARD_SIZE; i++)
                for(int j=0; j < BOARD_SIZE; j++) {
                    if(board[i][j].type == player || board[i][j].type == (player == WHITE ? WHITE_KING : BLACK_KING))
                        player_has_pieces = true;
                    if(board[i][j].type == ai_player || board[i][j].type == (ai_player == WHITE ? WHITE_KING : BLACK_KING))
                        ai_has_pieces = true;
                }

            if(!player_has_pieces && ai_has_pieces) {
                cout << "ИИ выиграл!\n";
            }
            else if(!ai_has_pieces && player_has_pieces) {
                cout << "Поздравляем! Вы выиграли!\n";
            }
            else {
                vector<pair<Position, Position>> player_moves = get_all_possible_moves(player);
                vector<pair<Position, Position>> ai_moves = get_all_possible_moves(ai_player);
                if(player_moves.empty() && ai_moves.empty())
                    cout << "Ничья!\n";
                else if(player_moves.empty())
                    cout << "У вас нет доступных ходов. ИИ выиграл!\n";
                else if(ai_moves.empty())
                    cout << "ИИ не может сделать ход. Вы выиграли!\n";
                else
                    cout << "Игра окончена.\n";
            }
            break;
        }

        if(player_turn_flag) {
            player_turn(player);
        } else {
            cout << "Ход ИИ...\n";
            ai_turn(ai_player);
        }

        player_turn_flag = !player_turn_flag;
    }

    display_board();
    cout << "Игра окончена.\n";

    return 0;
}
