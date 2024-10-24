#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "tokenizer.h"

// forward declaration of source_file function since compiler gave warning of implicit declaration.
int source_file(char *filename, char *prev[MAX_INPUT_SIZE]);

char parse_redirect(char **tokens, char *filepath)
{
  int i = 0;
  char redirect_char = 0;

  while (tokens[i] != NULL)
  {
    if (tokens[i][0] == '>' || tokens[i][0] == '<')
    {
      redirect_char = tokens[i][0];
    }
    else if (redirect_char)
    {
      strcpy(filepath, tokens[i]);

      tokens[i - 1] = NULL;

      return redirect_char;
    }

    i++;
  }
  return 1;
}

int tokens_to_command(int start, int t, char **tokens, char **first_cmd, char *sep, char delimiter)
{
  if (tokens == NULL || start >= t)
  {
    return -1;
  }

  int c = 0;
  for (int i = start; i < t; i++)
  {
    if (tokens[i] == NULL || tokens[i][0] == delimiter)
    {
      *sep = tokens[i][0];
      return c;
    }

    if (first_cmd != NULL)
    {
      first_cmd[c] = malloc(MAX_INPUT_SIZE * sizeof(char));
      strcpy(first_cmd[c++], tokens[i]);
    }
    else
    {
      c++;
    }
  }

  return c;
}

int tokens_to_commands(int t, char **tokens, char ***commands, char delimiter)
{
  int count = 0;
  int i = 0;
  char *sep = malloc(sizeof(char));
  int result = 0;

  while (result != -1)
  {
    *sep = ' ';
    // call with NULL just to count the tokens for allocation
    result = tokens_to_command(i, t, tokens, NULL, sep, delimiter);

    if (result > 0)
    {
      // now we can allocate memory exactly for number of tokens +1 for Null Terminator
      commands[count] = malloc((result + 1) * sizeof(char *));
      tokens_to_command(i, t, tokens, commands[count], sep, delimiter);
      commands[count][result] = NULL;
    }

    if (result != -1)
    {
      i += result;
      count++;
    }

    if (*sep != ' ')
    {
      i++;
    }
  }

  free(sep);
  return count;
}

int read_input(FILE *input_stream, char *input, char **tokens)
{
  // initialize the tokens array to NULL to avoid uninitialized memory access
  for (int i = 0; i < MAX_INPUT_SIZE; i++)
  {
    tokens[i] = NULL;
  }

  // read from stdin and print error if null
  if (fgets(input, MAX_INPUT_SIZE, input_stream) == NULL)
  {
    return -1;
  }

  // Remove the newline character from the input
  input[strcspn(input, "\n")] = 0;

  int i = 0;
  int t = 0;

  // tokenize loop
  while (input[i] != '\0')
  {
    // if input is a whitespace whitespace, skip it
    if (is_space_char(input[i]))
    {
      i++;
      continue; // continue to next iteration of while
    }

    // if input is a quotation
    if (input[i] == '"')
    {
      // quoted string returns the size of str including quotes so we can set iterator variable to += the returned value
      tokens[t] = malloc(MAX_INPUT_SIZE * sizeof(char));
      i += read_quoted_string(&input[i], tokens[t++]);
      continue;
    }

    // if input is a special char
    if (is_special_char(input[i]))
    {
      tokens[t] = malloc(MAX_INPUT_SIZE * sizeof(char));
      tokens[t++] = &input[i];
      i++;
      continue;
    }

    // if input is a regular word
    tokens[t] = malloc(MAX_INPUT_SIZE * sizeof(char));
    i += read_word(&input[i], tokens[t++]);
  }

  tokens[t] = NULL; // null terminate the end of tokens
  return t;
}

int exit_graceful()
{
  printf("Bye bye.\n");
  return 0;
}

// prints the help message
void print_help()
{
  printf("Mini-shell Help Page\n");
  printf("* This mini-shell was created by Nate Sawant and Alex Kouyoumjian *\n\n");

  printf("Built-in Commands:\n");
  printf("  cd <directory>      : Changes the current working directory to the specified <directory>.\n");
  printf("  source <filename>   : Executes a script file. Reads and processes each line in <filename> as a command.\n");
  printf("  prev                : Re-executes the previous command.\n");
  printf("  exit or Ctrl+D      : Exits the shell.\n\n");

  printf("Special Operators:\n");
  printf("  ;                   : Command separator, allows multiple commands to be executed in sequence.\n");
  printf("  > <file>            : Redirects the standard output to <file>.\n");
  printf("  < <file>            : Redirects the standard input from <file>.\n");
  printf("  |                   : Pipes the output of one command into the input of another.\n\n");

  printf("Shell Behavior:\n");
  printf("  * Supports commands up to 255 characters.\n");
  printf("  * Strings enclosed in double quotes (\" \") are treated as a single argument, even with spaces or special characters.\n");
  printf("  * Each command runs in the foreground by default until it completes.\n");
  printf("  * If a command is not recognized, '[command]: command not found' is printed.\n");
}

void change_directory(char *dir)
{
  chdir(dir);
}

int set_prev_command(char **tokens, char **prev)
{
  // if there is no prev, print error
  if (prev[0] == NULL)
  {
    return -1;
  }

  // initialize the tokens array to avoid uninitialized access
  for (int i = 0; i < MAX_INPUT_SIZE; i++)
  {
    tokens[i] = NULL;
  }

  int t = 0;
  // copy over previous command
  for (int i = 0; i < MAX_INPUT_SIZE; i++)
  {
    if (prev[i] != NULL)
    {
      if (tokens[i] != NULL)
      {
        free(tokens[i]);
      }
      tokens[i] = malloc(MAX_INPUT_SIZE * sizeof(char));
      strcpy(tokens[i], prev[i]);
      free(prev[i]);

      t++;
    }
    else
    {
      tokens[i] = NULL;
      break;
    }
  }

  return t;
}

// Processes and executes a command.
// Returns 0 to exit gracefully, 1 if successful (but do not exit).
int process_command(char **tokens, int t, char *prev[MAX_INPUT_SIZE])
{
  // If the input is "prev" set tokens to previous input by copying each prev token into tokens
  if (strcmp(tokens[0], "prev") == 0)
  {
    t = set_prev_command(tokens, prev);

    if (t == -1)
    {
      return 1;
    }
  }

  if (t > 0)
  {
    // This stores the current input to be the previous.
    // 1 if tokens are remaining. If a NULL is encountered, we know there are no more tokens and this gets set to 0.
    int are_tokens_remaining = 1;
    for (int i = 0; i < MAX_INPUT_SIZE; i++)
    {
      if (tokens[i] == NULL)
      {
        are_tokens_remaining = 0;
      }

      if (are_tokens_remaining)
      {
        // malloc space for the previous command
        prev[i] = malloc(MAX_INPUT_SIZE * sizeof(char));
        strcpy(prev[i], tokens[i]);
      }
      else
      {
        prev[i] = NULL;
      }
    }

    if (strcmp(tokens[0], "exit") == 0)
    {
      // input is "exit" so we return 0 to exit gracefully
      return 0;
    }
    else if (strcmp(tokens[0], "help") == 0)
    {
      // input is "help"
      print_help();
    }
    else if (strcmp(tokens[0], "cd") == 0)
    {
      change_directory(tokens[1]);
    }
    else if (strcmp(tokens[0], "source") == 0)
    {
      // input is "source"
      if (t >= 2)
      {
        // call source_file helper with the filename
        if (source_file(tokens[1], prev) == -1)
        {
          // if there was an error print it here
          printf("failed to execute script: %s\n", tokens[1]);
        }
      }
      else // else there is no second argument so print error
      {
        printf("source: missing filename argument\n");
      }
    }
    else
    {
      char ***commands = malloc(MAX_INPUT_SIZE * sizeof(char **));
      int num_cmds = tokens_to_commands(t, tokens, commands, '|');

      // Piping
      int pipefds[2 * (num_cmds - 1)];
      pid_t pids[num_cmds];

      for (int i = 0; i < num_cmds - 1; i++)
      {
        if (pipe(pipefds + i * 2) < 0)
        {
          perror("pipe");
          exit(1);
        }
      }

      // Split up each subcommand based on redirects
      for (int i = 0; i < num_cmds; i++)
      {
        pids[i] = fork();
        char filepath[MAX_INPUT_SIZE];
        char op = parse_redirect(commands[i], filepath);

        // printf("Operator: %c\n", op);
        // printf("Filepath: %s\n", filepath);

        if (pids[i] == 0)
        {
          if (op == '>')
          {
            // Close standard out
            if (close(1) == -1)
            {
              perror("Error closing stdout");
              exit(1);
            }

            // Create the file, truncate it if it exists
            int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

            // The open file should replace standard out
            assert(fd == 1);
          }
          else if (op == '<')
          {
            // Close standard in
            if (close(0) == -1)
            {
              perror("Error closing stdin");
              exit(1);
            }

            // Open the file for reading
            int fd = open(filepath, O_RDONLY);

            // The open file should replace standard in
            assert(fd == 0);
          }

          // Get input from prev if not first
          if (i > 0)
          {
            dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
          }

          // Send output to next if not last
          if (i < num_cmds - 1)
          {
            dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
          }

          for (int j = 0; j < 2 * (num_cmds - 1); j++)
          {
            close(pipefds[j]);
          }

          if (execvp(commands[i][0], commands[i]) == -1)
          {
            printf("%s: command not found\n", commands[i][0]);
          }
        }
      }

      // free allocated memory
      for (int i = 0; i < num_cmds; i++)
      {
        for (int j = 0; commands[i][j] != NULL; j++)
        {
          free(commands[i][j]);
        }
        free(commands[i]);
      }
      free(commands);

      for (int i = 0; i < 2 * (num_cmds - 1); i++)
      {
        close(pipefds[i]);
      }

      for (int i = 0; i < num_cmds; i++)
      {
        waitpid(pids[i], NULL, 0);
      }
    }
  }

  return 1;
}

// sources the given file
int source_file(char *filename, char *prev[MAX_INPUT_SIZE])
{
  // open the file in read mode
  FILE *file = fopen(filename, "r");
  if (file == NULL)
  {
    // if file is null an error will be printed in main since return value is -1
    return -1;
  }

  char input[MAX_INPUT_SIZE];
  char *tokens[MAX_INPUT_SIZE];

  // read each line from the file
  while (1)
  {
    // call read_input giving it the file instead of stdin
    int t = read_input(file, input, tokens);
    if (t == -1)
    {
      // if t is -1 then end of file
      break;
    }

    tokens[t] = NULL; // null terminate end of tokens

    if (t > 0)
    {
      // if process_command results in 0, the command instructs us to exit here.
      if (process_command(tokens, t, prev) == 0)
      {
        fclose(file);
        return exit_graceful();
      }
    }
  }

  fclose(file);
  return 1;
}

int main(int argc, char **argv)
{
  printf("Welcome to mini-shell.\n");

  // pointer to the previous inputted command (NULL if no previous command)
  // it is initialize to null to avoid jumping with unit value error (from valgrind) (same with *tokens below)
  char *prev[MAX_INPUT_SIZE] = {NULL};

  while (1)
  {
    printf("shell $ ");

    char input[MAX_INPUT_SIZE];
    char *tokens[MAX_INPUT_SIZE] = {NULL};
    int t = read_input(stdin, input, tokens);

    // run child process
    tokens[t] = NULL;

    if (t == -1)
    {
      // case for error reading the input or ctrl+d. We want to exit.
      printf("\n");
      return exit_graceful();
    }
    if (t == 0)
    {
      // if t is empty then there is no new input so just continue
      continue;
    }
    else if (t > 0)
    {
      // initialize to null to avoid jumping with unit value error (from valgrind)
      char *cmd_tokens[MAX_INPUT_SIZE] = {NULL};
      char *sep = malloc(sizeof(char));
      int cmd_t = 0;
      int i = 0;

      while (cmd_t >= 0)
      {
        *sep = ' ';

        cmd_t = tokens_to_command(i, t, tokens, cmd_tokens, sep, ';');
        i += cmd_t;
        cmd_tokens[cmd_t] = NULL;

        if (cmd_t == 0)
        {
          break;
        }
        int processed = 0;

        if (*sep == ';')
        {
          i++;
        }

        processed = process_command(cmd_tokens, cmd_t, prev);

        if (processed == 0)
        {
          // free sep and tokens
          free(sep);
          for (int j = 0; j < MAX_INPUT_SIZE; j++)
          {
            if (tokens[j] != NULL)
            {
              free(tokens[j]);
            }
          }
          return exit_graceful();
        }
      }
      // free sep after while loop
      free(sep);
    }
  }
  // Free prev[] before exiting.
  for (int i = 0; i < MAX_INPUT_SIZE; i++)
  {
    if (prev[i] != NULL)
    {
      free(prev[i]);
    }
  }

  return 0;
}