#include "lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <pwd.h>
#include <signal.h>
#include "readline/history.h"
#include "readline/readline.h"

/*Initialize the shell for use. Allocate all data structures
* Grab control of the terminal and put the shell in its own
* process group. NOTE: This function will block until the shell is
* in its own program group. Attaching a debugger will always cause
* this function to fail because the debugger maintains control of
* the subprocess it is debugging.
*/
void sh_init(struct shell *sh) {

    // Allocate memory for the shell structure
    sh->shell_terminal = STDIN_FILENO;
    sh->shell_is_interactive = isatty(sh->shell_terminal);
    sh->prompt = get_prompt("MY PROMPT");
    sh->shell_pgid = getpid();
    
    // Set the shell prompt
    if(sh->shell_is_interactive){
        while(tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp()))
            kill(-sh->shell_pgid, SIGTTIN);
    }

    // Set the shell process group ID
    //sh->shell_pgid = getpid();

    // Set the shell process group ID
    if(setpgid(sh->shell_pgid, sh->shell_pgid) < 0){
        perror("setpgid");
        exit(1);
    }
}


// Destroy shell. Free any allocated memory and resources and exit normally.
void sh_destroy(struct shell *sh) {

    // Free any allocated memory
    free(sh->prompt);

    // Exit the shell, don't want this
    // This caused too many problems, saw it already in main
    //exit(0);
}


/*Set the shell prompt. This function will attempt to load a prompt
* from the requested environment variable, if the environment variable is
* not set a default prompt of "shell>" is returned. This function calls
* malloc internally and the caller must free the resulting string.*/
char *get_prompt(const char *env) {
    char *prompt = getenv(env);
    if (prompt == NULL) {
        prompt = "shell>";
    }
    // Returns a copy of the prompt
    return strdup(prompt);
}


/*Changes the current working directory of the shell. Uses the linux system
* call chdir. With no arguments the users home directory is used as the
* directory to change to.*/
int change_dir(char **dir) {
    if (dir[1] == NULL){
        // No argument, change to home directory
        char *myHome = getenv("HOME");
        if (myHome == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return -1;
        }
        // If HOME is not set, get the home directory from passwd
        // struct passwd *pw = getpwnam(getlogin());
        if (!myHome) {
            struct passwd *pw = getpwuid(getuid());
            myHome = pw ? pw->pw_dir : NULL;
        }
        return chdir(myHome);
    }
    return chdir(dir[1]);
    // If chdir fails, it will return -1 and set errno
}


/*Convert line read from the user into to format that will work with
* execvp. We limit the number of arguments to ARG_MAX loaded from sysconf.
* This function allocates memory that must be reclaimed with the cmd_free
* function.*/
char **cmd_parse(char const *line) {
    // Check for NULL
    if (line == NULL) {
        return NULL;
    }

    // Allocate memory for the command line
    char **argv = malloc(sizeof(char*) * sysconf(_SC_ARG_MAX));
    if (argv == NULL) {
        perror("malloc");
        return NULL;
    }

    // Tokenize the line
    //this part might need to be changed to work?
    int i = 0;
    char *line_copy = strdup(line);
    char *saveptr;
    char *token = strtok_r(line_copy, " ", &saveptr);
    
    // Loop through the tokens and add them to the argv array
    while (token != NULL && i < sysconf(_SC_ARG_MAX) - 1) {
        argv[i++] = strdup(token);
        token = strtok_r(NULL, " ", &saveptr);
    }
    
    // Null terminate the array
    argv[i] = NULL;

    // Free the line copy
    free(line_copy);

    return argv;
}


//Free the line that was constructed with parse_cmd
void cmd_free(char **line) {
    // Check for NULL
    if (line == NULL) {
        return;
    }

    // Free each argument
    for (int i = 0; line[i]; i++) {
        free(line[i]);
    }

    // Free the array itself
    free(line);
}


/*Trim the whitespace from the start and end of a string.
* For example " ls -a " becomes "ls -a". This function modifies
* the argument line so that all printable chars are moved to the
* front of the string*/
char *trim_white(char *line) {
    if (line == NULL) {
        return NULL;
    }

    // Pointer to the end of the string
    char *final;

    // Trim leading space
    while (isspace((unsigned char)*line)) line++;

    if (*line == 0)  // All spaces?
        return line;

    // Trim trailing space
    final = line + strlen(line) - 1;
    while (final > line && isspace((unsigned char)*final)) final--;

    // Write new null terminator
    *(final + 1) = '\0';

    return line;
}


/*Takes an argument list and checks if the first argument is a
* built in command such as exit, cd, jobs, etc. If the command is a
* built in command this function will handle the command and then return
* true. If the first argument is NOT a built in command this function will
* return false.*/
bool do_builtin(struct shell *sh, char **argv) {
    
    // Check for NULL or empty command
    if (argv == NULL || argv[0] == NULL) {
        return false;
    }

    // Check for built-in commands
    if(strcmp(argv[0], "exit") == 0) {
        exit(0);
    }

    // Check for change directory command
    if(strcmp(argv[0], "cd") == 0) {
        return change_dir(argv) == 0;
    }

    if(strcmp(argv[0], "jobs") == 0) {
        HIST_ENTRY **the_list;
        //register int i;

        //the_list = history_list ();
        the_list = history_list();
          if (the_list)
            for (int i = 0; the_list[i]; i++)
              printf ("%d: %s\n", i + history_base, the_list[i]->line);
        return true;
    }

    // If no built-in command was found, return false
    return false;
}


// Parse command line args from the user when the shell was launched
void parse_args(int argc, char **argv) {
    // If the version flag is found, print the version and exit
    for (int i = 0; i < argc; i++) {
        // Check for version flag
        if(strcmp(argv[i], "-v") == 0){
            // Print version and exit
            printf("The Shell Version is: %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MAJOR);
            exit(0);
        }
    }
}
