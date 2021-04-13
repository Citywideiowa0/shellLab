#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/*Copied and edited Professor Curtsinger's strpbrk code from the assignment page*/

// +------------------------+---------------------------------------------
// | Globals & Declarations |
// +------------------------+

// This is the maximum number of arguments your shell should handle for one command
#define MAX_ARGS 128

void executeFunction(char**, int, char);

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
    // Catch and print background processes (if they have completed)
    int status;
    int child_ID;
    while ((child_ID = (waitpid(0, &status, WNOHANG))) > 0) {
      printf("[background process %d exited with status %d]\n", child_ID, WEXITSTATUS(status));
    }

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

    // -------- Split input line into seperate commands  --------

    char* command = line;
    while (true) {
      // Locate next semicolon or ampersand delimeter
      char* delim_position = strpbrk(command, ";&");

      // Variable to keep track of whether the delimeter was an ampersand or semicolon
      char delim = ';';

      if (delim_position == NULL) { /* There were no more delimeters */

        // Exit if there was no command
        if (strcmp(command, "") == 0) {
          break;
        }
      } else { /* There was a delimeter */
        // First, save it
        delim = *delim_position;

        // Overwrite the delimeter with a null terminator so we can print just this fragment
        *delim_position = '\0';
      }

      // --------- Split individual commands into arguments ----------

      // Create an empty array of char*'s (strings)
      char** myArgs = (char**)malloc(sizeof(char*) * MAX_ARGS);

      // 'size' tracks the number of arguments (including the command itself) from the input
      int size = 0;

      // Using strsep to read each argument seperated by a space or newline
      char* arg = strsep(&command, " \n");
      while (arg != NULL) {
        if (arg[0] != '\0') {   /* excluding empty commands/arguments */
          myArgs[size++] = arg; /*Saving the argument in the array*/
        }
        arg = strsep(&command, " \n"); /* Readng next argument */
      }
      myArgs[size] = NULL; /* execvp requires NULL at the end of the array */

      // If empty command, nothing to execute, so move to prompting next input
      if (myArgs[0] == NULL) {
        break;
      }

      // Checking for special commands
      if (strcmp(myArgs[0], "cd") == 0) { /* If command is change directory (cd) */
        if (chdir(myArgs[1]) != 0) {
          perror("Changing directory not possible \n");
          exit(1);
        }
      } else if (strcmp(myArgs[0], "exit") == 0) { /* If command is exit */
        exit(0);
      } else if (strcmp(myArgs[0], "") != 0) { /* If command is not empty */
        executeFunction(myArgs, size, delim);
      }

      // Prompt new input
      if (delim_position == NULL) {
        break;
      }
      // Move our current position in the string to one character past the delimeter
      command = delim_position + 1;
    }  // end command while loop
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

void executeFunction(char** myArgs, int len, char delim) {
  int id = fork();

  if (id < 0) { /* If fork failed */
    perror("fork() failed");
    exit(1);

  } else if (id == 0) { /* If child process*/
    execvp(myArgs[0], myArgs);

    // If process reaches here then execvp failed so display error and exit
    perror("exec failure");
    exit(EXIT_FAILURE);

  } else { /* If parent process */
    // If delimeter is not an ampersand, then we should wait for the child to finish
    if (delim == ';' || delim == '\0') {
      // Wait for child process and if there is an error, display it and exit
      int status;
      if (waitpid(id, &status, 0) == -1) {
        perror("Wait pid error\n");
        exit(EXIT_FAILURE);
      }

      // Print message to indicate that the command has been executed
      printf("[%s exited with status %d]\n", myArgs[0], WEXITSTATUS(status));
    }
  }
}
