

/*****************************************************************************************
  
  File             shell.c
  Author           Patrick Ryan
  College          Boston University: Metropolitian College
  Course           CS575 (Operating Systems)
  Date             7/31/2018

  Compile          gcc -o shell  shell.c
  Run              ./shell
  Known Bugs:      None



*******************************************************************************************

C functions and operations used in this project:

*******************************************************************************************


Functions:


- fopen() FILE *fopen(const char *filename, const char *mode) opens the filename pointed to, by filename using the given mode.

-strtok() char *strtok(char *str, const char *delim) breaks string str into a series of tokens using the delimiter delim.

-chdir() - change working directory

-strcmp():  compares the string pointed to, by str1 to the string pointed to by str2.

-ftell():  ftell(FILE *stream): returns the current file position of the given stream.

-fgets(): char *fgets(char *str, int n, FILE *stream) reads a line from the specified stream and stores it into the string pointed to by str. It stops when either (n-1) characters are read, the newline character is read, or the end-of-file is reached, whichever comes first.

-fclose(): int fclose(FILE *stream) closes the stream. All buffers are flushed.

-perror(): void perror(const char *str) prints a descriptive error message to stderr. First the string str is printed, followed by a colon then a space.

-fseek(): The C library function int fseek(FILE *stream, long int offset, int whence) sets the file position of the stream to the given offset.

-free(): The C library function void free(void *ptr) deallocates the memory previously allocated by a call to calloc, malloc, or realloc.

-ftell(): long int ftell(FILE *stream) returns the current file position of the given stream.

-malloc():  The C library function void *malloc(size_t size) allocates the requested memory and returns a pointer to it.

-ctime(): char *ctime(const time_t *timer) returns a string representing the localtime based on the argument timer.

The returned string has the following format: Www Mmm dd hh:mm:ss yyyy, where Www is the weekday, Mmm the month in letters, dd the day of the month, hh:mm:ss the time, and yyyy the year.

-getchar() int getchar(void) gets a character (an unsigned char) from stdin. This is equivalent to getc with stdin as its argument.


//*****************************************************************************************
  
Macros

//*****************************************************************************************

-NULL This macro is the value of a null pointer constant.

-EXIT_FAILURE  This is the value for the exit function to return in case of failure.

-EXIT_SUCCESS  This is the value for the exit function to return in case of success.



//*****************************************************************************************
Operations:
//*****************************************************************************************



-&function: returns the actual address of the variable.
-int *p; or int* p;   p is a pointer to an int
-int **p; or int** p;   p is a pointer to a pointer to an int. 
-x = *p;   Read: Assign to x the value pointed to by p.
-WIFEXITED: returns a nonzero value if the child process terminated normally with exit or _exit.




******************************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <malloc.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>        


#define HISTORY_FILE "/home/patrick/Desktop/history.txt"
#define TEST_BUFFER_A 256 
#define TEST_BUFFER_B 1024 
#define K_BUFF 1024
#define RL_BUFFER_SIZE 1024
#define TOKEN_BUFFER_SIZE 64
#define TOKEN_DELIM " \t\r\n\a"




//******************************************************************************
// INTERNAL COMMANDS AND SETUP
//******************************************************************************


// Primary declarations
int time_cmd(char **args);
int help_cmd(char **args);
int cd_cmd(char **args);
int print_hist_cmd(char **args);
int delete_all_history(char **args);
int exit_cmd(char **args);
int run_prev_history(char **args);
char **split_line(char *line);
int run_target_history(char **args, int bang_num);

// Internal commands
char *builtin_str[] = {
    "cd",
    "help",
    "time",
    "history",
    "exit"
    };

// Addresses of internal commands
int (*builtin_func[])(char **) = {
    &cd_cmd,
    &help_cmd,
    &time_cmd,
    &print_hist_cmd,
    &exit_cmd
    };


// Count of built in functions (5)
int builtin_func_count()
{
  return sizeof(builtin_str) / sizeof(char *);
}

// change directory
int cd_cmd(char **args)
{
  if (args[1] == NULL)
  {
    fprintf(stderr, "WARNING: expected location argument to \"cd\"\n");
  }
  else
  {
    if (chdir(args[1]) != 0)
    {perror("WARNING: (chdir(args[1]) != 0) ");
    }
  }
  return 1;
}

// help menu
int help_cmd(char **args)
{
  int i;

  printf("\n");
  printf("****************************************************\n");
  printf("****************************************************\n");
  printf("\n");
  printf("  HELP MENU:\n");
  printf("\n");
  printf("  Type program names and arguments, and hit enter.\n");
  printf("  The current internal programs are as follows:\n");
  printf("\n");
  printf("****************************************************\n");
  printf("****************************************************\n");
  printf("\n");
  for (i = 0; i < builtin_func_count(); i++)  // print functions
  {
    printf(" - %s\n", builtin_str[i]);
  }
  printf("\n");
  printf("****************************************************\n");
  printf("\n");
  return 1;
}


// time command
int time_cmd(char **args)
{
  time_t mytime = time(NULL);
  char *time_str = ctime(&mytime);
  time_str[strlen(time_str) - 1] = 0;
  printf("Current time: %s", time_str);
  printf("\n");
  return 1;
}


// exit
int exit_cmd(char **args)
{
  printf("Exit Command Detected...\n");
  printf("Exiting...\n");
  return 0;  // exit main loop by returning zero
}




// ******************************************************************************
// HISTORY ASSOCIATED FUNCTIONS AND COMMANDS
// ******************************************************************************


// add new entry to the the history record
void add_new_entry(char *line)
{
  FILE *record;
  record = fopen(HISTORY_FILE, "a");  //"a" = append
  if (record == NULL)
  {
    perror("Warning");
  }
  else
  {
    if (line != NULL && strlen(line) != 0 && strcmp(line, "history") != 0 && strcmp(line, "!!") != 0 && line[0] != '!')
    {
      fprintf(record, "%s", line);  // print new item in file
      fprintf(record, "\n"); 
    }
  }
  fclose(record);  // close connection to file
}


// print all entrys in the history record
int print_hist_cmd(char **args)
{
  char line[100][100];
  int x;
  int y = 0;
  int total = 0;
  int temp = 0;



  FILE * record = NULL;
  record = fopen(HISTORY_FILE, "r");  // open connection to history record

  if (record != NULL)  
  {
    fseek(record, 0, SEEK_END);
    x = ftell(record);  
  //  printf("%i", x);
  //  printf("%i", &x);
    if (x == 0)  
    {
      printf("History.txt file empty.  Please enter more commands.\n");
      return 1;
    }
  }

  fclose(record);  // close record stream
  free(record);  // deallocate record
  record = fopen(HISTORY_FILE, "r");  //

  while (fgets(line[y], 100, record))
  {
    line[y][strlen(line[y]) - 1] = 0;    // 'remove \n' 
    y++;
  }

  total = y;
  printf("\n");
  printf("****************************************************\n");
  for (y = 0; y < total; ++y)
  {
    printf("%d  ", y + 1);
    printf("%s", line[y]);
    printf("\n");
  }
  printf("****************************************************\n");
  printf("\n");
  fclose(record);  // close history file
  return 1;
}

// delete all entries in the history record
int delete_all_history(char **args)
{
  FILE *record;
  record = fopen(HISTORY_FILE, "w");
  fclose(record);
  return 1;
}



// launch program (internal/external)
int launch(char **args)
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
    perror("WARNING!: Error forking!!");
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




// parse and intercept history commands ("history" , "!!", "!n")
int parse_line_args(char **args)
{
  int k;
  int temp;

 // printf("%i_%i_%i", args[0][0], args[0][1], args[0][2]);

  if (args[0] == NULL)
  {

     fprintf(stderr, "WARNING: args[0] == NULL ");  
     return 1;
  }

  for (k = 0; k < builtin_func_count(); k++)
  {

    if (strcmp(args[0], builtin_str[k]) == 0)  // if first argument == string_name
    {
      return (*builtin_func[k])(args);  // return the (i)th function for launching
    }

    if (strcmp(args[0], "!!") == 0) // if args[0] is a double bang
    {
      return run_prev_history(args);  // return command to rerun last item
    }

    // differentiate from !! since 33_33_0 -> "!!"
    // e.g. !1 = 33_49_0, !4 = 33_52_0, 
    if (args[0][0] == 33 && args[0][1] > 48)
//   if (args[0][0] == 33 && args[0][1] != 33 && args[0][1] != 0 && args[0][2] == 0)
    {
      temp = atoi(&args[0][1]);  // convert the "n" to an interger
      return run_target_history(args, temp);  // run selected command in history
    }
  }
  return launch(args);
}


// *************************************************************************************


// find most recent history entry in record file and launch
int run_prev_history(char **args)
{
  char line[100][100];
  char **temp;
  int x = 0;
  int y = 0;
  int total = 0;

  FILE *record = NULL;
  record = fopen(HISTORY_FILE, "r");
  while (fgets(line[x], 100, record))
  {
    line[x][strlen(line[x]) - 1] = 0;
    x++;
  }

  for (y = 0; y < builtin_func_count(); y++)
  {
    if (strcmp(builtin_str[y], line[x - 1]) == 0)
    {
      return (*builtin_func[y])(args);
    }
  }

  temp = split_line(line[x - 1]);
  fclose(record);
  return launch(temp);
}


// *******************************************************************************************

// use index to find target history and launch
int run_target_history(char **args, int index)
{

  FILE *record = NULL;
  int x = 0;
  int y = 0;
  char line[100][100];
  // int total = 0;
  char **temp;

  record = fopen(HISTORY_FILE, "r");
  while (fgets(line[x], 100, record) && x < index)
  {
    line[x][strlen(line[x]) - 1] = 0;
    x++;
  }
  for (y = 0; y < builtin_func_count(); y++)
  {
    if (strcmp(builtin_str[y], line[x - 1]) == 0)
    {
      return (builtin_func[y])(args);
    }
  }

  temp = split_line(line[x - 1]);
  fclose(record);
  return launch(temp);
}

//*********************************************************************************************8


// read lines from user input
char *read_line() 
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

  while (1) // perp
  {
    c = getchar();

    if (c == EOF)
    {
      exit(EXIT_SUCCESS);  // exit upon end of file
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


// split lines into individual commands and arguments
char **split_line(char *line)
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

//***************************************************************************************


// primary loop
void shell_loop(void)
{
  char *line;
  char **args;
  int x;
  delete_all_history(args);
  do
  {
    printf("PR> ");
    line = read_line();  //read line
    add_new_entry(line);    // add line to history.txt file
    args = split_line(line);  // split line into commands
    x = parse_line_args(args);
    free(line);  // deallocate line
    free(args);  // deallocate args
  } while (x);
}


//**************************************************************************************


// main loop
int main(int argc, char **argv)
{
  // Load configuration files

  // Run command loop.
  shell_loop();

  // Perform shutdown/cleanup activities.

  return EXIT_SUCCESS;
}
