#define _ISOC99_SOURCE
#define _DEFAULT_SOURCE

#include "msgs.h"
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LEN 10

void print_help_info() {
  char *msg = FORMAT_MSG("exit", EXIT_HELP_MSG);
  write(STDOUT_FILENO, msg, strlen(msg));
  msg = FORMAT_MSG("pwd", PWD_HELP_MSG);
  write(STDOUT_FILENO, msg, strlen(msg));
  msg = FORMAT_MSG("cd", CD_HELP_MSG);
  write(STDOUT_FILENO, msg, strlen(msg));
  msg = FORMAT_MSG("help", HELP_HELP_MSG);
  write(STDOUT_FILENO, msg, strlen(msg));
  msg = FORMAT_MSG("history", HISTORY_HELP_MSG);
  write(STDOUT_FILENO, msg, strlen(msg));
}
void sigint_handler(int signum) {
  write(STDOUT_FILENO, "\n", strlen("\n")); // not finished
  print_help_info();
  return;
}

// const int max_len = 10; // history variables
char *cmd_history[MAX_LEN];
int history_count = -1;
int current = 0;

void remove_oldest_record() {
  if (history_count > 0) { // maybe add circular array
    free(cmd_history[0]);
    for (int i = 1; i < current; i++) {
      cmd_history[i - 1] = cmd_history[i];
    }
    current--;
  }
}

void add_to_history(char *input) {
  if (current >= MAX_LEN) {
    remove_oldest_record();
  }

  cmd_history[current] = strdup(input); // REMEMBER to free
  current++;
  history_count++;
}

void free_history() {
  for (int i = 0; i < current; i++) {
    free(cmd_history[i]);
    cmd_history[i] = NULL;
  }
}

int main() {
  struct sigaction act;
  act.sa_handler = sigint_handler;
  act.sa_flags = 0;
  sigemptyset(&act.sa_mask);

  sigaction(SIGINT, &act, NULL); // signal handler stuff

  int size = 50;
  char buffer[size];
  char *arg[10];
  char *pathname = NULL;

  for (int j = 0; j < 10; j++) {
    arg[j] = NULL;
  }

  while (true) {

    char *prev_path = pathname;

    int i = 1;
    pathname = getcwd(NULL, 0);
    if (pathname == NULL) {
      const char *msg = FORMAT_MSG("shell", GETCWD_ERROR_MSG);
      write(STDERR_FILENO, msg, strlen(msg));
      exit(0);
    }

    strcat(pathname, "$ ");
    write(STDOUT_FILENO, pathname, strlen(pathname));

    size_t len = read(STDIN_FILENO, buffer, sizeof(buffer));
    if (len == -1) {
      const char *msg = FORMAT_MSG("shell", READ_ERROR_MSG);
      write(STDERR_FILENO, msg, strlen(msg));
      exit(0);
    }

    int wstatus = 0;
    while (waitpid(-1, &wstatus, WNOHANG) > 0) {
    } // waitpid returns zero when no child to wait on

    buffer[len - 1] = '\0';
    add_to_history(buffer);

    char *saveptr;
    char *token = strtok_r(buffer, " ", &saveptr);
    arg[0] = token;
    int index = 0;

    while (token != NULL) {
      token = strtok_r(NULL, " ", &saveptr);
      arg[i] = token;
      index = i;
      i++;
    }
    bool bg_process = false;
    int length = strlen(arg[index - 1]);    // offset the index var
    if (strcmp(arg[index - 1], "&") == 0) { // check if it's bg execution
      // arg[index - 1][length - 1] = '\0';
      arg[index - 1] = NULL;
      bg_process = true;
    }
    // arg[9] = NULL;

    /*for (int j = 0; j < 10; j++) {
      printf("token: %s\n", arg[j]); // for testing purposes
    }*/

    pid_t pid = -2;
    if (strcmp(arg[0], "history") == 0) {
      int tmp_count = history_count;
      // char history_msg[100];
      int j = current;
      for (int i = 0; i < current; i++) {
        /*char tmp_string[50];
        sprintf(tmp_string, "%d", tmp_count);
        history_msg = strcat(tmp_string, "\t");
        history_msg = strcat(history_msg, cmd_history[j]);*/
        char history_msg[100];
        snprintf(history_msg, sizeof(history_msg), "%d\t%s\n", tmp_count,
                 cmd_history[j - 1]);
        write(STDOUT_FILENO, history_msg, strlen(history_msg));
        // write(STDOUT_FILENO, "\n", strlen("\n"));
        j--;
        tmp_count--;
      }

    } else if (strcmp(arg[0], "exit") == 0) {
      if (index > 1) {
        const char *msg = FORMAT_MSG("exit", TMA_MSG);
        write(STDERR_FILENO, msg, strlen(msg));
      } else {
        free_history();
        exit(0);
      }
    } else if (strcmp(arg[0], "pwd") == 0) {
      if (index > 1) {
        const char *msg = FORMAT_MSG("pwd", TMA_MSG);
        write(STDERR_FILENO, msg, strlen(msg));
      } else {
        pathname = getcwd(NULL, 0);
        if (pathname == NULL) {
          const char *msg = FORMAT_MSG("pwd", GETCWD_ERROR_MSG);
          write(STDERR_FILENO, msg, strlen(msg));
        }
        strcat(pathname, "\n");
        write(STDOUT_FILENO, pathname, strlen(pathname));
      }
    } else if (strcmp(arg[0], "cd") == 0) {
      uid_t uid = getuid();
      struct passwd *pw = getpwuid(uid);
      char *home_dir = pw->pw_dir;

      if (arg[1] == NULL) {
        arg[1] = home_dir; // no path arguement

      } else if (arg[1][0] == '~') {
        arg[1] = strcat(home_dir, arg[1] + 1);
      } else if (arg[1][0] == '-') {
        if (prev_path != NULL) {
          int len = strlen(prev_path);
          prev_path[len - 2] = '\0';
          arg[1] = prev_path; // what happens if there is no previous directory?
          printf("prev_path: %s\n", prev_path);
        }
      }

      if (chdir(arg[1]) == -1) {
        const char *msg = FORMAT_MSG("cd", CHDIR_ERROR_MSG);
        write(STDERR_FILENO, msg, strlen(msg));
      }
      if (index > 2) {
        const char *msg = FORMAT_MSG("cd", TMA_MSG);
        write(STDERR_FILENO, msg, strlen(msg));
      }
    } else if (strcmp(arg[0], "help") == 0) {
      if (index > 2) {
        const char *msg = FORMAT_MSG("help", TMA_MSG);
        write(STDERR_FILENO, msg, strlen(msg));
      } else if (index == 1) {
        char *msg = FORMAT_MSG("exit", EXIT_HELP_MSG);
        write(STDOUT_FILENO, msg, strlen(msg));
        msg = FORMAT_MSG("pwd", PWD_HELP_MSG);
        write(STDOUT_FILENO, msg, strlen(msg));
        msg = FORMAT_MSG("cd", CD_HELP_MSG);
        write(STDOUT_FILENO, msg, strlen(msg));
        msg = FORMAT_MSG("help", HELP_HELP_MSG);
        write(STDOUT_FILENO, msg, strlen(msg));
        msg = FORMAT_MSG("history", HISTORY_HELP_MSG);
        write(STDOUT_FILENO, msg, strlen(msg));
      }

      else if (strcmp(arg[1], "exit") == 0) {
        const char *msg = FORMAT_MSG("exit", EXIT_HELP_MSG);
        write(STDOUT_FILENO, msg, strlen(msg));
      }

      else if (strcmp(arg[1], "pwd") == 0) {
        const char *msg = FORMAT_MSG("pwd", PWD_HELP_MSG);
        write(STDOUT_FILENO, msg, strlen(msg));
      }

      else if (strcmp(arg[1], "cd") == 0) {
        const char *msg = FORMAT_MSG("cd", CD_HELP_MSG);
        write(STDOUT_FILENO, msg, strlen(msg));
      }

      else if (strcmp(arg[1], "help") == 0) {
        const char *msg = FORMAT_MSG("help", HELP_HELP_MSG);
        write(STDOUT_FILENO, msg, strlen(msg));
      }

      else if (strcmp(arg[1], "history") == 0) {
        const char *msg = FORMAT_MSG("history", HISTORY_HELP_MSG);
        write(STDOUT_FILENO, msg, strlen(msg));
      }

      else { // help external command
        /*char help_buffer[20];
        snprintf(help_buffer, sizeof(help_buffer), "%s", arg[1]);
        const char *msg = FORMAT_MSG(help_buffer, EXTERN_HELP_MSG);
        write(STDOUT_FILENO, msg, strlen(msg));*/

        char *e_cmd_help_msg =
            strcat(arg[1], ": external command or application\n");
        write(STDOUT_FILENO, e_cmd_help_msg, strlen(e_cmd_help_msg));
      }
    }

    else {
      pid = fork(); // put this in if else if
    }

    if (pid > 0 && !bg_process) { // parent
      if (waitpid(pid, &wstatus, 0) == -1) {
        const char *msg = FORMAT_MSG("shell", WAIT_ERROR_MSG);
        write(STDERR_FILENO, msg, strlen(msg));
        exit(0);
      }

    } else if (pid == 0) { // child
      if (execvp(arg[0], arg) == -1) {
        const char *msg = FORMAT_MSG("shell", EXEC_ERROR_MSG);
        write(STDERR_FILENO, msg, strlen(msg));
        exit(0);
      }
    } else if (pid == -1) { // fork fails
      const char *msg = FORMAT_MSG("shell", FORK_ERROR_MSG);
      write(STDERR_FILENO, msg, strlen(msg));
      exit(0);
    }
  }
}
