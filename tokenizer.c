#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "tokenizer.h"

#define MAX_INPUT_SIZE 255

// Helper function to check if a char ch is special
int is_special_char(char ch) {
  return (ch == '(' || ch == ')' || ch == '<' || ch == '>' || ch == ';' || ch == '|');
}

// Helper function to check if a char ch is a space or tab
int is_space_char(char ch) {
  return (ch == ' ' || ch == '\t' || ch == '\n');
}

// Helper function to tokenize characters into a word by reading until it hits a special char.
// Returns: length of the word
int read_word(const char *input, char *output) {
  int i = 0;
  // read until we hit a special char, quote, or space
  while (input[i] != '\0' && !is_space_char(input[i]) && !is_special_char(input[i]) && input[i] != '"') {
    output[i] = input[i];
    i++;
  }
  output[i] = '\0'; // add the terminating byte
  return i;
}

// Function to tokenize quoted strings
// Returns: length of string including its quotations
int read_quoted_string(const char *input, char *output) {
  // start iterator variable i at 1 so we skip the starting quote
  int i = 1; 
  int j = 0; // index of output
  // while i is not at null terminator or ", add char at i to output
  while (input[i] != '\0' && input[i] != '"') {
    output[j++] = input[i++];
  }
  output[j] = '\0'; // add the terminating byte
  // return length of string adding one for the closing "
  // ternary operation in case user forgets to add closing quote, just returns i without adding 1.
  return (input[i] == '"') ? i + 1 : i;
}
