#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h> 
#include <cstring>

const char event[] = "Event";
const char variant[] = "Variant";
const char event_of_interest[] = "Rated Blitz game";
const unsigned int game_log_freq = 1000;
const char whiteElo[] = "WhiteElo";
const char blackElo[] = "BlackElo";
const int high_elo = 2000;

const char castle_king[] = "O-O";
const char castle_queen[] = "O-O-O";


const size_t max_moves = 15;

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

unsigned long long int read_game_body(
    char* line,
    size_t len,
    unsigned long long* white_board,
    unsigned long long* black_board) {
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
  while (i < len && line[i] != '\n') {
    if (isspace(line[i])) {
      if (j == 0) { 
        i++;
        continue;
      }
      move[j] = '\0';
      if (!ignore) {
        // Process the Game.
        if (is_in_check) {
          if (is_white_move) {
            // printf("white king %s: %li\n", move, white_king);
            white_board[white_king]++;    
          } else {
            // printf("black king %s: %li\n", move, black_king);
            black_board[black_king]++;    
          }
        }
        if (is_white_move) {
          maybe_update_king_pos(move, &white_king, is_white_move);
        } else {
          maybe_update_king_pos(move, &black_king, is_white_move);
        }
        is_in_check = is_check(move);
        // printf("white king %s: %li\n", move, white_king);
        // printf("black king %s: %li\n", move, black_king);

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

  // When the game is finished and user is in check.
  if (is_in_check) {
    if (is_white_move) {
      white_board[white_king]++;    
    } else {
      black_board[black_king]++;    
    }
  }
  game_round = game_round >> 1;
  // printf("%i\n", game_round);
  return game_round;
}

void add_board(unsigned long long* total,
    unsigned long long* board) {
  int i = 0;
  for(; i < 64; ++i) {
    total[i] += board[i];
  }
}

bool read_game(
    char **line,
    size_t* len,
    unsigned long long* number_of_games,
    unsigned long long* total_moves, 
    unsigned long long* high_number_of_games,
    unsigned long long* high_total_moves,
    unsigned long long* total_white_board,
    unsigned long long* total_black_board,
    unsigned long long* high_white_board,
    unsigned long long* high_black_board) {

  char key[50];
  char value[200];
  int nread;
  bool skip_game = false;

  unsigned long long game_white_board[64] = {0};
  unsigned long long game_black_board[64] = {0};

  bool is_white_high_rating = false;
  bool is_black_high_rating = false;
  unsigned long long moves;

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
    printf("High ELO: %llu: %llu\n", *high_number_of_games, *high_total_moves);
    printf("%llu: %llu\n", *number_of_games, *total_moves);
    return false;
  }

  moves = read_game_body(*line, nread, game_white_board, game_black_board);

  if (is_white_high_rating && is_black_high_rating) {
    (*high_number_of_games)++;
    (*high_total_moves) += moves;
    add_board(high_white_board, game_white_board);
    add_board(high_black_board, game_black_board);
  }

  add_board(total_white_board, game_white_board);
  add_board(total_black_board, game_black_board);

  // Read Game.
  (*number_of_games)++;
  (*total_moves) += moves;
  if (*number_of_games % game_log_freq == 0) {
    printf("%llu: %llu\n", *number_of_games, *total_moves);
  }
  return true;
}

void print_board(unsigned long long* board) {
  int i = 7;
  int j;

  for(;i>=0; i--) {
    j = 0;
    for(;j<8; j++) {
      printf("%llu, ", board[i*8 + j]);
    }
    printf("\n");
  }
}

int main() {
  char *line = NULL;
  size_t len = 0;
  // Counts number of games and number of moves.
  unsigned long long int number_of_games = 0;
  unsigned long long int total_moves = 0;
  unsigned long long int high_number_of_games = 0;
  unsigned long long int high_total_moves = 0;

  // Counts the squares where checks happened.
  unsigned long long total_white_board[64] = {0};
  unsigned long long total_black_board[64] = {0};
  unsigned long long high_white_board[64] = {0};
  unsigned long long high_black_board[64] = {0};
  while(
      read_game(&line,
                &len,
                &number_of_games,
                &total_moves,
                &high_number_of_games,
                &high_total_moves,
                total_white_board,
                total_black_board,
                high_white_board,
                high_black_board));
  // print_board(white_board);
  printf("High Black Board\n");
  print_board(high_black_board);
  printf("High White Board\n");
  print_board(high_white_board);
  printf("Black Board\n");
  print_board(total_black_board);
  printf("White Board\n");
  print_board(total_white_board);
  free(line);
  exit(EXIT_SUCCESS);
}
