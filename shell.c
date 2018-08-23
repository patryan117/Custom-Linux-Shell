/***************************************************************************/ /**
  File         shell.c
  Author       Patrick Ryan
  College      Boston University: Metropolitian College
  Course       CS575 (Operating Systems)
  Date         7/31/2018

  Compile      gcc -o shell  shell.c
  Run          ./shell



*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

/*
  DECLARE INTERNAL SHELL COMMANDS:
 */

int time_cmd(char **args);
int help_cmd(char **args);
int cd_cmd(char **args);
int show_history_cmd(char **args);
int clear_history_cmd(char **args);
int exit_cmd(char **args);

#define BUF 128 /* can change the buffer size as well */
#define TOT 10  /* change to accomodate other sizes, change ONCE here */
#define RL_BUFFER_SIZE 1024
#define TOKEN_BUFFER_SIZE 64
#define TOKEN_DELIM " \t\r\n\a"

/*
  List of builtin commands, followed by their corresponding functions.
 */

char *builtin_str[] = {
    "cd",
    "help",
    "time",
    "history",
    "clear_hist",
    "exit"};

int (*builtin_func[])(char **) = {
    &cd_cmd,
    &help_cmd,
    &time_cmd,
    &show_history_cmd,
    &clear_history_cmd,
    &exit_cmd};

int builtin_func_count()
{
  return sizeof(builtin_str) / sizeof(char *);
}

int cd_cmd(char **args)
{
  if (args[1] == NULL)
  {
    fprintf(stderr, "WARNING: expected argument to \"cd\"\n");
  }
  else
  {
    if (chdir(args[1]) != 0)
    {
      perror("WARNING");
    }
  }
  return 1;
}

int help_cmd(char **args)
{
  int i;

  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < builtin_func_count(); i++)
  {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int time_cmd(char **args)
{
  time_t mytime = time(NULL);
  char *time_str = ctime(&mytime);
  time_str[strlen(time_str) - 1] = '\0';
  printf("Current Time : %s\n", time_str);
  return 1;
}

void add_hist(char *line)
{
  FILE *hist_file;
  char *h = "history";
  char *b = "!!";
  char *bn = "!";
  hist_file = fopen("/tmp/history.txt", "a");
  if (hist_file == NULL)
  {
    perror("Error opening file");
  }
  else
  {
    if (line != NULL && strlen(line) != 0 && strcmp(line, h) != 0 && strcmp(line, b) != 0 && line[0] != '!')
    {
      fprintf(hist_file, "%s\n", line);
    }
  }
  fclose(hist_file);
}


int clear_history_cmd(char **args)
{
  FILE *hist_file;
  hist_file = fopen("/tmp/history.txt", "w");
  fclose(hist_file);
  return 1;
}


int show_history_cmd(char **args)
{

  char line[TOT][BUF];
  FILE *hist_file = NULL;
  int i = 0;
  int total = 0;
  int size;

  hist_file = fopen("/tmp/history.txt", "r");
  if (NULL != hist_file)
  {
    fseek(hist_file, 0, SEEK_END);
    size = ftell(hist_file);
    if (0 == size)
    {
      printf("History is empty\n");
      return 1;
    }
  }
  fclose(hist_file);
  free(hist_file);
  hist_file = fopen("/tmp/history.txt", "r");
  while (fgets(line[i], BUF, hist_file))
  {
    /* get rid of ending \n from fgets */
    line[i][strlen(line[i]) - 1] = '\0';
    i++;
  }

  total = i;

  for (i = 0; i < total; ++i)
  {
    printf("%d  ", i + 1);
    printf("%s\n", line[i]);
  }
  fclose(hist_file);

  return 1;
}

// exit program by returning zero

int exit_cmd(char **args)
{
  return 0;
}

int burt_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0)
  {
    // Child process
    if (execvp(args[0], args) == -1)
    {
      perror("WARNING");
    }
    exit(EXIT_FAILURE);
  }
  else if (pid < 0)
  {
    // Error forking
    perror("burt");
  }
  else
  {
    // Parent process
    do
    {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

// Bring these functions up so I can use them;
int run_last_hist(char **args);
char **burt_split_line(char *line);
int run_pick_hist(char **args, int bang_num);

int burt_execute(char **args)
{
  int i;
  int bang;
  char *bangbang = "!!";
  int val_frst = args[0][0];
  int val_scnd = args[0][1];
  int val_thrd = args[0][2];

  if (args[0] == NULL)
  {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < builtin_func_count(); i++)
  {
    if (strcmp(args[0], builtin_str[i]) == 0)
    {
      return (*builtin_func[i])(args);
    }
    if (strcmp(args[0], bangbang) == 0)
    {
      return run_last_hist(args);
    }
    // testing for value of after bang
    if (val_frst == 33 && val_scnd != 33 && val_scnd != 0 && val_thrd == 0)
    {
      bang = atoi(&args[0][1]);
      return run_pick_hist(args, bang);
    }
  }
  return burt_launch(args);
}

int run_last_hist(char **args)
{
  char line[TOT][BUF];
  FILE *hist_file = NULL;
  int i = 0;
  int x = 0;
  int total = 0;
  char **hist_args;

  hist_file = fopen("/tmp/history.txt", "r");
  while (fgets(line[i], BUF, hist_file))
  {
    /* get rid of ending \n from fgets */
    line[i][strlen(line[i]) - 1] = '\0';
    i++;
  }

  for (x = 0; x < builtin_func_count(); x++)
  {
    if (strcmp(builtin_str[x], line[i - 1]) == 0)
    {
      return (*builtin_func[x])(args);
    }
  }
  hist_args = burt_split_line(line[i - 1]);
  fclose(hist_file);
  return burt_launch(hist_args);
}

int run_pick_hist(char **args, int bang_num)
{
  char line[TOT][BUF];
  FILE *hist_file = NULL;
  int i = 0;
  int x = 0;
  // int total = 0;
  char **hist_args;

  hist_file = fopen("/tmp/history.txt", "r");
  while (fgets(line[i], BUF, hist_file) && i < bang_num)
  {
    /* get rid of ending \n from fgets */
    line[i][strlen(line[i]) - 1] = '\0';
    i++;
  }
  for (x = 0; x < builtin_func_count(); x++)
  {
    if (strcmp(builtin_str[x], line[i - 1]) == 0)
    {
      return (*builtin_func[x])(args);
    }
  }
  hist_args = burt_split_line(line[i - 1]);
  fclose(hist_file);
  return burt_launch(hist_args);
}

char *burt_read_line()
{
  int bufsize = RL_BUFFER_SIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;
  if (!buffer)
  {
    fprintf(stderr, "WARNING: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1)
  {
    // Read a character
    c = getchar();

    if (c == EOF)
    {
      exit(EXIT_SUCCESS);
    }
    else if (c == '\n')
    {
      buffer[position] = '\0';
      return buffer;
    }
    else
    {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize)
    {
      bufsize += RL_BUFFER_SIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer)
      {
        fprintf(stderr, "WARNING: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}



char **burt_split_line(char *line)
{
  int bufsize = TOKEN_BUFFER_SIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char *));
  char *token, **tokens_backup;
  char hist_token;

  if (!tokens)
  {
    fprintf(stderr, "WARNING: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, TOKEN_DELIM);
  while (token != NULL)
  {
    tokens[position] = token;
    position++;
    // printf("%s\n", tokens[position]);

    if (position >= bufsize)
    {
      bufsize += TOKEN_BUFFER_SIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char *));
      if (!tokens)
      {
        free(tokens_backup);
        fprintf(stderr, "WARNING: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, TOKEN_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}



void shell_loop(void)
{
  char *line;
  char **args;
  int status;
  clear_history_cmd(args);
  do
  {
    printf("PR> ");
    line = burt_read_line();
    add_hist(line);
    args = burt_split_line(line);
    status = burt_execute(args);

    free(line);
    free(args);
  } while (status);
}

int main(int argc, char **argv)
{
  // Load config files, if any.

  // Run command loop.
  shell_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}