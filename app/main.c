#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "../src/lab.h"

static void explain_waitpid(int status)
{
  if (!WIFEXITED(status))
  {
    fprintf(stderr, "Child exited with status %d\n", WEXITSTATUS(status));
  }
  if (WIFSIGNALED(status))
  {
    fprintf(stderr, "Child exited via signal %d\n", WTERMSIG(status));
  }
  if (WIFSTOPPED(status))
  {
    fprintf(stderr, "Child stopped by %d\n", WSTOPSIG(status));
  }
  if (WIFCONTINUED(status))
  {
    fprintf(stderr, "Child was resumed by delivery of SIGCONT\n");
  }
}

// Set up signal handlers
void setup_signal_handlers(void){
  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
}


int main(int argc, char *argv[])
{
  parse_args(argc, argv);
  struct shell sh;
  sh_init(&sh);

  // Set up signal handlers
  setup_signal_handlers();

  char *line = (char *)NULL;

  // Set the prompt
  while ((line = readline(sh.prompt)))
  {
    // do nothing on blank lines don't save history or attempt to exec
    line = trim_white(line);
    if (!*line)
    {
      free(line);
      continue;
    }
    add_history(line);
    // check to see if we are launching a built in command
    char **cmd = cmd_parse(line);
    if (!do_builtin(&sh, cmd))
    {
      pid_t pid = fork();
      // If fork failed we are in trouble!
      if (pid == 0)
      {
        /*This is the child process*/
        pid_t child = getpid();
        setpgid(child, child);
        tcsetpgrp(sh.shell_terminal, child);
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        execvp(cmd[0], cmd);
        // If execvp failed we are in trouble!
        perror("execvp failed");
        exit(EXIT_FAILURE);
      }
      else if (pid < 0)
      {
        // If fork failed we are in trouble!
        perror("fork return < 0 Process creation failed!");
        abort();
      }
      /*
      This is in the parent put the child process into its own
      process group and give it control of the terminal
      to avoid a race condition
      */
      // printf("shell:%d , child%d\n",sh.shell_pgid, pid);
      setpgid(pid, pid);
      tcsetpgrp(sh.shell_terminal, pid);
      int status;
      int rval = waitpid(pid, &status, 0);
      if (rval == -1)
      {
        fprintf(stderr, "Wait pid failed with -1\n");
        explain_waitpid(status);
      }

      cmd_free(cmd);
      // get control of the shell
      tcsetpgrp(sh.shell_terminal, sh.shell_pgid);
    }
    free(line);
  }
  // Might be good to have this here :)
  sh_destroy(&sh);

  exit(EXIT_SUCCESS);
}