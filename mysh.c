#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// +------------------------+---------------------------------------------
// | Globals & Declarations |
// +------------------------+

// This is the maximum number of arguments your shell should handle for one command
#define MAX_ARGS 128

void printArr(char**, int);

void forkFunction(char**, int);

// +------+---------------------------------------------
// | Main |
// +------+

int main(int argc, char** argv) {
  // If there was a command line option passed in, use that file instead of stdin
  if (argc == 2) {
    // Try to open the file
    int new_input = open(argv[1], O_RDONLY);
    if (new_input == -1) {
      fprintf(stderr, "Failed to open input file %s\n", argv[1]);
      exit(1);
    }

    // Now swap this file in and use it as stdin
    if (dup2(new_input, STDIN_FILENO) == -1) {
      fprintf(stderr, "Failed to set new file as input\n");
      exit(2);
    }
  }

  char* line = NULL;     // Pointer that will hold the line we read in
  size_t line_size = 0;  // The number of bytes available in line

  // Loop forever
  while (true) {
    // Print the shell prompt
    printf("$ ");

    // Get a line of stdin, storing the string pointer in line
    if (getline(&line, &line_size, stdin) == -1) {
      if (errno == EINVAL) {
        perror("Unable to read command line");
        exit(2);
      } else {
        // Must have been end of file (ctrl+D)
        printf("\nShutting down...\n");

        // Exit the infinite loop
        break;
      }
    }

    // ---------------  OUR CODE ---------------------------

    // -------- Split into Command Modules --------
    char* command = line;
    while (true) {
      // Call strpbrk to find the next occurrence of a delimeter
      char* delim_position = strpbrk(command, ";");

      if (delim_position == NULL) {
        // There were no more delimeters.
        if (strcmp(command, "") == 0) {
          break;
        }
      } else {
        // There was a delimeter. First, save it.
        // char delim = *delim_position;

        // Overwrite the delimeter with a null terminator so we can print just this fragment
        *delim_position = '\0';

        // printf("The fragment %s was found, followed by '%c'.\n", command, delim);
      }

      // --------- Split Cmmd Module into Arguments ----------

      // Create an empty array of char*'s
      char** myArgs = (char**)malloc(sizeof(char*) * MAX_ARGS);
      // 'size' tracks the number of tokens from the input
      int size = 0;
      char* arg = strsep(&command, " \n");
      while (arg != NULL) {
        if (arg[0] != '\0') { /* excluding empty commands/arguments */
          myArgs[size++] = arg;
        }
        arg = strsep(&command, " \n");
      }
      myArgs[size] = NULL;  // execvp requires NULL at the end of the array

      if (strcmp(myArgs[0], "cd") == 0) {
        if (chdir(myArgs[1]) != 0) {
          perror("Changing directory not possible \n");
          exit(1);
        }
      } else if (strcmp(myArgs[0], "exit") == 0) {
        exit(0);
      } else if (strcmp(myArgs[0], "") != 0) {
        forkFunction(myArgs, size);
      }

      // Move our current position in the string to one character past the delimeter
      if (delim_position == NULL) break;
      command = delim_position + 1;
    }  // end command module while loop
  }    // end Professor's read input while loop

  // If we read in at least one line, free this space
  if (line != NULL) {
    free(line);
  }

  return 0;
}

// +--------------------+---------------------------------------------
// | Helper Procedures  |
// +--------------------+

void printArr(char** arr, int len) {
  for (int i = 0; i < len; i++) {
    printf("args[%d]=[%s]\n", i, arr[i]);
  }
}

void forkFunction(char** myArgs, int len) {
  // MAKE FORKING A FUNCTION
  // check for exit and cd (special cases), and maybe blank line
  int id = fork();

  // if fork failed
  if (id < 0) {
    perror("fork() failed");
    exit(1);

    // if child process
  } else if (id == 0) {
    // the p means to use the version of exec that finds the file using a path
    //   configuration (basically searches for the command in all the configurred paths)
    // 'bin' is conventional name for directory holding executable

    // char **newArgs[] = {"ls","ls",NULL};
    execvp(myArgs[0], myArgs);
    perror("exec failure");
    exit(EXIT_FAILURE);

    // if parent
  } else {
    int status;
    // Note: rc stands for return code; can also just use if statement
    //  wait doesn't return 0 but the id of the child, so not equal
    //    to zero or child's id
    int rc_wait = wait(&status);
    rc_wait = 1;
    /*
    printf("We're here\n");
    int pid;
    while ((pid = waitpid(id, &status, WNOHANG)) != 0) {
      printf("[background process %d exited with status %d]", pid, WEXITSTATUS(status));
    }
    printf("Do we get here?\n");
    */

    // Note:
    //   waitpid() wait for a specific process
    //   wNoHang flag

    printf("[%s exited with status %d]\n", myArgs[0], status);
  }
}
