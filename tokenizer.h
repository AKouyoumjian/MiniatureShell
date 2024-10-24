#define MAX_INPUT_SIZE 255

// Helper function to check if a char ch is special
int is_special_char(char ch);

// Helper function to check if a char ch is a space or tab
int is_space_char(char ch);

// Helper function to tokenize characters into a word by reading until it hits a special char.
// Returns: length of the word
int read_word(const char *input, char *output);

// Function to tokenize quoted strings
// Returns: length of string including its quotations
int read_quoted_string(const char *input, char *output);