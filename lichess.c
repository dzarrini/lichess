#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h> 
#include <stdbool.h>
#include <string.h>

const char event[] = "Event";
const char variant[] = "Variant";
const char event_of_interest[] = "Rated Blitz game";
const unsigned int game_log_freq = 1000000;
const unsigned long long max_limit = -1;
const char whiteElo[] = "WhiteElo";
const char blackElo[] = "BlackElo";
const int high_elo = 2000;

const char castle_king[] = "O-O";
const char castle_queen[] = "O-O-O";

const char white_win_str[] = "1-0";
const char black_win_str[] = "0-1";
const char draw_str[] = "1/2-1/2";

const size_t max_moves = 15;

struct ChessDataState {
  // Game Results.
  unsigned long long white_win;
  unsigned long long black_win;
  unsigned long long draw;

  // Checkmates.
  unsigned long long white_checkmate;
  unsigned long long black_checkmate;

  // Counts number of games and number of moves.
  unsigned long long number_of_games;
  unsigned long long total_moves;

  // Counts the squares where checks happened.
  unsigned long long white_board[64];
  unsigned long long black_board[64];

  // Deadliest square.
  unsigned long long white_capture[64];
  unsigned long long black_capture[64];

  // Promotions.
  unsigned long long white_promotion[8];
  unsigned long long black_promotion[8];
};

void initialize(struct ChessDataState* chess_data_state) {
  // initalize.
  chess_data_state->white_win = 0;
  chess_data_state->black_win = 0;
  chess_data_state->draw = 0;
  chess_data_state->white_checkmate = 0;
  chess_data_state->black_checkmate = 0;
  chess_data_state->number_of_games = 0;
  chess_data_state->total_moves = 0;

  memset(chess_data_state->white_board, 0, sizeof(unsigned long long) * 64);
  memset(chess_data_state->black_board, 0, sizeof(unsigned long long) * 64);
  memset(chess_data_state->white_capture, 0, sizeof(unsigned long long) * 64);
  memset(chess_data_state->black_capture, 0, sizeof(unsigned long long) * 64);
  memset(chess_data_state->black_promotion, 0, sizeof(unsigned long long) * 8);
  memset(chess_data_state->white_promotion, 0, sizeof(unsigned long long) * 8);
}

void add_data(
    struct ChessDataState* total_chess_data,
    struct ChessDataState* current_chess_data) {
  total_chess_data->white_win += current_chess_data->white_win;
  total_chess_data->black_win += current_chess_data->black_win;
  total_chess_data->draw += current_chess_data->draw;
  total_chess_data->white_checkmate += current_chess_data->white_checkmate;
  total_chess_data->black_checkmate += current_chess_data->black_checkmate;
  total_chess_data->number_of_games += current_chess_data->number_of_games;
  total_chess_data->total_moves += current_chess_data->total_moves;

  size_t i = 0;
  for(;i < 64; ++i) {
    total_chess_data->white_board[i] += current_chess_data->white_board[i];
    total_chess_data->black_board[i] += current_chess_data->black_board[i];
    total_chess_data->white_capture[i] += current_chess_data->white_capture[i];
    total_chess_data->black_capture[i] += current_chess_data->black_capture[i];
  }

  i = 0;
  for(;i < 8; ++i) {
    total_chess_data->white_promotion[i] += current_chess_data->white_promotion[i];
    total_chess_data->black_promotion[i] += current_chess_data->black_promotion[i];
  }
}

void read_header(char* line, char* key, char* value) {
  int val = sscanf(line, "[ %s \"%[^\"]\" ]", key, value);
  if (val == 1) {
    perror("Failed to read header.");
    exit(EXIT_FAILURE);
  }
}

bool is_check(char* move) {
  size_t i = 0;
  for (;i < max_moves; ++i) {
    if (move[i] == '+') return true;
  }
  return false;
}

bool is_checkmate(char* move) {
  size_t i = 0;
  for (;i < max_moves; ++i) {
    if (move[i] == '#') return true;
  }
  return false;
}

void maybe_update_king_pos(
    char* move,
    size_t* current_pos,
    bool is_white_move) {
  size_t i = 0;
  if (strcmp(move, castle_king) == 0) {
    if (is_white_move) {
      *current_pos = 6;
    } else {
      *current_pos = 62;
    }
    return;
  }
  if (strcmp(move, castle_queen) == 0) {
    if (is_white_move) {
      *current_pos = 2;
    } else {
      *current_pos = 58;
    }
    return;
  }
  for (;i< max_moves - 2; ++i) {
    if (move[i] == '\0') { 
      return;
    }
    if (move[i] == 'K') {
      if (move[i+1] != 'x') {
        *current_pos = (move[i+1] - 'a') + (move[i+2] - '1') * 8;
      } else {
        *current_pos = (move[i+2] - 'a') + (move[i+3] - '1') * 8;
      }
    }
  }
  return;
}

int was_capture(char* move) {
  size_t i = 0;
  for (;i< max_moves; ++i) {
    if (move[i] == '\0') { 
      return -1;
    }
    if (move[i] == 'x') {
      return (move[i+1] - 'a') + (move[i+2] - '1') * 8;
    }
  }
  return -1;
}

int was_promotion(char* move) {
  size_t i = 0;
  for (;i< max_moves; ++i) {
    if (move[i] == '\0') { 
      return -1;
    }
    if (move[i] == '=') {
      // printf("%s\n", move);
      return (move[i-2] - 'a');
    }
  }
  return -1;
}

void read_game_body(
    char* line,
    size_t len,
    struct ChessDataState* chess_data_state) { 
  size_t i = 0;
  size_t j = 0;
  char move[max_moves];
  bool ignore = true; // game round.
  bool is_white_move = true;
  unsigned long game_round = 1;

  // King position.
  size_t white_king = 4;
  size_t black_king = 60;
  bool is_in_check = false;

  // Capture.
  int capture = -1;
  int promotion = -1;
  while (i < len && line[i] != '\n') {
    if (isspace(line[i])) {
      if (j == 0) { 
        i++;
        continue;
      }
      move[j] = '\0';
      if (!ignore) {
        // Process the Game.
        // printf("%s\n", move);
        if (is_in_check) {
          if (is_white_move) {
            chess_data_state->white_board[white_king]++;    
          } else {
            chess_data_state->black_board[black_king]++;    
          }
        }
        if (is_white_move) {
          maybe_update_king_pos(move, &white_king, is_white_move);
        } else {
          maybe_update_king_pos(move, &black_king, is_white_move);
        }
        if (is_white_move && is_checkmate(move)) {
          // If its white move and checkmate occured. Then black got checkmated.
          chess_data_state->black_checkmate++;
        } else if (is_checkmate(move)) {
          chess_data_state->white_checkmate++;
        }

        capture = was_capture(move);
        if (capture >= 0) {
          if (is_white_move) {
            chess_data_state->white_capture[capture]++;
          } else {
            chess_data_state->black_capture[capture]++;
          }
        }
        promotion = was_promotion(move);
        if (promotion >= 0) {
          if (is_white_move) {
            chess_data_state->white_promotion[promotion]++;
          } else {
            // printf("%s\n", move);
            chess_data_state->black_promotion[promotion]++;
          }
        }
        is_in_check = is_check(move);
        is_white_move = !is_white_move;
      }
      // 1. white_move 1... black_move. Ignores 1. and 1...
      ignore = !ignore; 
      if (ignore) {
        game_round++;
      }
      j = 0;
      i++;
      continue;
    }
    if (line[i] == '{') {
      while(line[i] != '}') i++;
      i++;
      continue;
    }
    move[j] = line[i];
    j++;
    i++;
  }

  if (strcmp(move, white_win_str) == 0) {
    chess_data_state->white_win++;
  } else if (strcmp(move, black_win_str) == 0) {
    chess_data_state->black_win++;
  } else {
    chess_data_state->draw++;
  }

  // When the game is finished and user is in check.
  if (is_in_check) {
    if (is_white_move) {
      chess_data_state->white_board[white_king]++;    
    } else {
      chess_data_state->black_board[black_king]++;    
    }
  }
  game_round = game_round >> 1;
  chess_data_state->total_moves = game_round;
  chess_data_state->number_of_games = 1;
  return;
}

bool read_game(
    char **line,
    size_t* len,
    struct ChessDataState* chess_data_state) {

  char key[50];
  char value[200];
  int nread;
  bool skip_game = false;
  bool is_white_high_rating = false;
  bool is_black_high_rating = false;

  struct ChessDataState current_game_data;
  initialize(&current_game_data);

  // Read Headers.
  while ((nread = getline(line, len, stdin)) != -1) {
    if (len == 0 || *line[0] == '\n') continue;
    if (*line[0] != '[') break;
    read_header(*line, key, value);
    if (strcmp(key, event) == 0) {
      skip_game = strcmp(value, event_of_interest) != 0;
    }
    if (strcmp(key, variant) == 0) {
      skip_game = true;
    }
    if (strcmp(key, whiteElo) == 0 && atoi(value) > high_elo) {
      is_white_high_rating = true;
    }
    if (strcmp(key, whiteElo) == 0 && atoi(value) > high_elo) {
      is_black_high_rating = true;
    }
  }

  if (skip_game) {
    return true;
  }

  if (nread == EOF) {
    return false;
  }

  read_game_body(
      *line,
      nread,
      &current_game_data);

  if (is_white_high_rating && is_black_high_rating) {
    add_data(chess_data_state, &current_game_data);
  }

  if (chess_data_state->number_of_games % game_log_freq == 0) {
    printf("Processed: %llu\n", chess_data_state->number_of_games / game_log_freq);
  }
  return true;
}

void print_board(unsigned long long* board) {
  int i = 7;
  int j;

  for(;i>=0; i--) {
    j = 0;
    for(;j<7; j++) {
      printf("%llu, ", board[i*8 + j]);
    }
    if(j ==7) {
      printf("%llu", board[i*8 + j]);
    }
    printf("\n");
  }
}

void print_promotion(unsigned long long* promotion) {
  int i = 0;

  for(;i<7; i++) {
    printf("%llu, ", promotion[i]);
  }
  printf("%llu\n", promotion[7]);
}

void print_data(struct ChessDataState* chess_data_state) {
  printf("Number of games: %llu\n", chess_data_state->number_of_games);
  printf("Total moves: %llu\n", chess_data_state->total_moves);
  printf("Black Board\n");
  print_board(chess_data_state->black_board);
  printf("White Board\n");
  print_board(chess_data_state->white_board);
  printf("White Win: %llu\n", chess_data_state->white_win);
  printf("Black Win: %llu\n", chess_data_state->black_win);
  printf("Draw Win: %llu\n", chess_data_state->draw);
  printf("White Checkmate: %llu\n", chess_data_state->white_checkmate);
  printf("Black Checkmate: %llu\n", chess_data_state->black_checkmate);
  printf("Black Capture\n");
  print_board(chess_data_state->black_capture);
  printf("White Capture\n");
  print_board(chess_data_state->white_capture);
  printf("White Promotion\n");
  print_promotion(chess_data_state->white_promotion);
  printf("Black Promotion\n");
  print_promotion(chess_data_state->black_promotion);
}

int main() {
  char *line = NULL;
  size_t len = 0;

  struct ChessDataState chess_data_state;
  initialize(&chess_data_state);

  while(
      read_game(&line,
                &len,
                &chess_data_state)) {
    if (max_limit > 0 && chess_data_state.number_of_games > max_limit) {
      break;
    }
  }
  free(line);
  print_data(&chess_data_state);
  exit(EXIT_SUCCESS);
}
