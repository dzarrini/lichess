#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h> 
#include <cstring>

const char event[] = "Event";
const char event_of_interest[] = "Rated Blitz game";

void read_header(char* line, char* key, char* value) {
  int val = sscanf(line, "[ %s \"%[^\"]\" ]", key, value);
  if (val == 1) {
    perror("Failed to read header.");
    exit(EXIT_FAILURE);
  }
}

void read_game_body(char* line, size_t len) {
  size_t i = 0;
  size_t j = 0;
  char move[15];
  bool ignore = true; // game round.
  unsigned int game_round = 1;
  while (i < len && line[i] != '\n') {
    if (isspace(line[i])) {
      if (j == 0) { 
        i++;
        continue;
      }
      move[j] = '\0';
      if (!ignore) {
        // printf("%s\n", move);
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
  game_round = game_round >> 1;
  printf("%i\n", game_round);
}

void read_game(char *line) {
  char key[50];
  char value[200];
  size_t len = 0;
  int nread;
  bool should_read_game = false;

  // Read Headers.
  while ((nread = getline(&line, &len, stdin)) != -1) {
    if (len == 0 || line[0] == '\n') continue;
    if (line[0] != '[') break;
    read_header(line, key, value);
    if (strcmp(key, event) == 0) {
      should_read_game = strcmp(value, event_of_interest) == 0;
    }
  }

  if (nread == EOF) {
    exit(EXIT_SUCCESS);
  }

  // Read Game.
  if (should_read_game) {
    read_game_body(line, len);
  }

  // free(line);
}

int main() {
  char *line = NULL;
  while(true) {
    read_game(line);
  }
}
