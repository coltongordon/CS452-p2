#include "lab.h"

/*Set the shell prompt. This function will attempt to load a prompt
* from the requested environment variable, if the environment variable is
* not set a default prompt of "shell>" is returned. This function calls
* malloc internally and the caller must free the resulting string.*/
char *get_prompt(const char *env) {
    const char *prompt = getenv(env);
    if (prompt == NULL) {
        prompt = "shell>";
    }
    char *result = malloc(strlen(prompt) + 1);
    if (result == NULL) {
        perror("malloc");
        return NULL;
    }
    strcpy(result, prompt);
    return result;
}


/*Changes the current working directory of the shell. Uses the linux system
* call chdir. With no arguments the users home directory is used as the
* directory to change to.*/
int change_dir(char **dir) {
    const char *home_dir = getenv("HOME");
    if (dir == NULL || *dir == NULL) {
        if (home_dir == NULL) {
            fprintf(stderr, "cd: HOME environment variable not set\n");
            return -1;
        }
        if (chdir(home_dir) != 0) {
            perror("cd");
            return -1;
        }
    } else {
        if (chdir(*dir) != 0) {
            perror("cd");
            return -1;
        }
    }
    return 0;
}


/*Convert line read from the user into to format that will work with
* execvp. We limit the number of arguments to ARG_MAX loaded from sysconf.
* This function allocates memory that must be reclaimed with the cmd_free
* function.*/
char **cmd_parse(char const *line) {
    int arg_max = sysconf(_SC_ARG_MAX);
    char **argv = malloc(sizeof(char *) * (arg_max + 1));
    if (argv == NULL) {
        perror("malloc");
        return NULL;
    }

    char *token;
    char *line_copy = strdup(line);
    if (line_copy == NULL) {
        perror("strdup");
        free(argv);
        return NULL;
    }

    int argc = 0;
    token = strtok(line_copy, " \t\n");
    while (token != NULL && argc < arg_max) {
        argv[argc] = strdup(token);
        if (argv[argc] == NULL) {
            perror("strdup");
            for (int i = 0; i < argc; i++) {
                free(argv[i]);
            }
            free(argv);
            free(line_copy);
            return NULL;
        }
        argc++;
        token = strtok(NULL, " \t\n");
    }
    argv[argc] = NULL;

    free(line_copy);
    return argv;
}


//Free the line that was constructed with parse_cmd
void cmd_free(char **line) {
    if (line != NULL) {
        free(*line);
        *line = NULL;
    }
}


/*Trim the whitespace from the start and end of a string.
* For example " ls -a " becomes "ls -a". This function modifies
* the argument line so that all printable chars are moved to the
* front of the string*/
char *trim_white(char *line) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*line)) line++;

    if (*line == 0)  // All spaces?
        return line;

    // Trim trailing space
    end = line + strlen(line) - 1;
    while (end > line && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = '\0';

    return line;
}


/*Takes an argument list and checks if the first argument is a
* built in command such as exit, cd, jobs, etc. If the command is a
* built in command this function will handle the command and then return
* true. If the first argument is NOT a built in command this function will
* return false.*/
bool do_builtin(struct shell *sh, char **argv) {
    if (argv[0] == NULL) {
        return false;
    }

    if (strcmp(argv[0], "exit") == 0) {
        sh_destroy(sh);
        return true;
    } else if (strcmp(argv[0], "cd") == 0) {
        if (argv[1] == NULL) {
            fprintf(stderr, "cd: expected argument\n");
        } else {
            if (chdir(argv[1]) != 0) {
                perror("cd");
            }
        }
        return true;
    } else if (strcmp(argv[0], "jobs") == 0) {
        // Handle jobs command
        // This is a placeholder for actual jobs handling code
        printf("jobs command executed\n");
        return true;
    }

    return false;
}


/*Initialize the shell for use. Allocate all data structures
* Grab control of the terminal and put the shell in its own
* process group. NOTE: This function will block until the shell is
* in its own program group. Attaching a debugger will always cause
* this function to fail because the debugger maintains control of
* the subprocess it is debugging.
*/
void sh_init(struct shell *sh) {
    if (sh == NULL) {
        return;
    }

    // Allocate memory for command history and current command
    sh->command_history = (char **)malloc(sizeof(char *) * COMMAND_HISTORY_SIZE);
    if (sh->command_history == NULL) {
        perror("Failed to allocate memory for command history");
        exit(1);
    }

    sh->current_command = (char *)malloc(sizeof(char) * COMMAND_BUFFER_SIZE);
    if (sh->current_command == NULL) {
        perror("Failed to allocate memory for current command");
        free(sh->command_history);
        exit(1);
    }

    // Initialize other shell fields as needed
    sh->history_count = 0;
    sh->current_command[0] = '\0';

    // Grab control of the terminal
    if (tcgetpgrp(STDIN_FILENO) != getpid()) {
        while (tcgetpgrp(STDIN_FILENO) != (sh->pgid = getpgrp())) {
            kill(-sh->pgid, SIGTTIN);
        }
    }

    // Put the shell in its own process group
    sh->pgid = getpid();
    if (setpgid(sh->pgid, sh->pgid) < 0) {
        perror("Couldn't put the shell in its own process group");
        exit(1);
    }

    // Take control of the terminal
    tcsetpgrp(STDIN_FILENO, sh->pgid);

    // Save default terminal attributes for restoration later
    tcgetattr(STDIN_FILENO, &sh->tmodes);
}


// Destroy shell. Free any allocated memory and resources and exit normally.
void sh_destroy(struct shell *sh) {
    if (sh == NULL) {
        return;
    }

    // Free any allocated memory within the shell structure
    if (sh->command_history) {
        free(sh->command_history);
    }

    if (sh->current_command) {
        free(sh->current_command);
    }

    // Free the shell structure itself
    free(sh);

    // Exit normally
    exit(0);
}


// Parse command line args from the user when the shell was launched
void parse_args(int argc, char **argv) {
    for (int i = 0; i < argc; i++) {
        printf("Argument %d: %s\n", i, argv[i]);
    }
}
