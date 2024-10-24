#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "tokenizer.h"

int main(int argc, char **argv) {
  char input[MAX_INPUT_SIZE];
  char buf[MAX_INPUT_SIZE];

  // read from stdin and print error if null
  if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
    printf("Error reading input\n");
    return 1;
  }

  int i = 0;
  while (input[i] != '\0') {
    // if input is a whitespace whitespace, skip it
    if (is_space_char(input[i])) {
        i++;
        continue; // continue to next iteration of while
    }

    // if input is a quotation
    if (input[i] == '"') {
      // quoted string returns the size of str including quotes so we can set iterator variable to += the returned value
      i += read_quoted_string(&input[i], buf);
      printf("%s\n", buf);
      continue;
    }

    // if input is a special char
    if (is_special_char(input[i])) {
      printf("%c\n", input[i]);
      i++;
      continue;
    }

    // if input is a regular word
    i += read_word(&input[i], buf);
    printf("%s\n", buf);
  }

  return 0;
}
