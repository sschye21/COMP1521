////////////////////////////////////////////////////////////////////////
// COMP1521 21t2 -- Assignment 2 -- shuck, A Simple Shell
// <https://www.cse.unsw.edu.au/~cs1521/21T2/assignments/ass2/index.html>
//
// Written by YOUR-NAME-HERE (z5555555) on INSERT-DATE-HERE.
//
// 2021-07-12    v1.0    Team COMP1521 <cs1521@cse.unsw.edu.au>
// 2021-07-21    v1.1    Team COMP1521 <cs1521@cse.unsw.edu.au>
//     * Adjust qualifiers and attributes in provided code,
//       to make `dcc -Werror' happy.
//

#include <sys/types.h>

#include <sys/stat.h>
#include <sys/wait.h>

#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// [[ TODO: put any extra `#include's here ]]
#include <spawn.h>
#include <ctype.h>
#include <glob.h>

// [[ TODO: put any `#define's here ]]
#define PATH_MAX 4096

//
// Interactive prompt:
//     The default prompt displayed in `interactive' mode --- when both
//     standard input and standard output are connected to a TTY device.
//
static const char *const INTERACTIVE_PROMPT = "shuck& ";

//
// Default path:
//     If no `$PATH' variable is set in Shuck's environment, we fall
//     back to these directories as the `$PATH'.
//
static const char *const DEFAULT_PATH = "/bin:/usr/bin";

//
// Default history shown:
//     The number of history items shown by default; overridden by the
//     first argument to the `history' builtin command.
//     Remove the `unused' marker once you have implemented history.
//
static const int DEFAULT_HISTORY_SHOWN = 10;

//
// Input line length:
//     The length of the longest line of input we can read.
//
static const size_t MAX_LINE_CHARS = 1024;

//
// Special characters:
//     Characters that `tokenize' will return as words by themselves.
//
static const char *const SPECIAL_CHARS = "!><|";

//
// Word separators:
//     Characters that `tokenize' will use to delimit words.
//
static const char *const WORD_SEPARATORS = " \t\r\n";

static void execute_command(char **words, char **path, char **environment);
static void do_exit(char **words);
static int is_executable(char *pathname);
static char **tokenize(char *s, char *separators, char *special_chars);
static void free_tokens(char **tokens);

// [[ TODO: put any extra function prototypes here ]]
void posix_helper(char **words, char *program, char **environment);
void print_history(int num, char *pathname);
static int find_len(char *pathname);
void print_exclam(int number, char *pathname, char **path, char **environment);
char **expand_filename(char **words);

int main (void)
{
    // Ensure `stdout' is line-matchesed for autotesting.
    setlinebuf(stdout);

    // Environment variables are pointed to by `environ', an array of
    // strings terminated by a NULL value -- something like:
    //     { "VAR1=value", "VAR2=value", NULL }
    extern char **environ;

    // Grab the `PATH' environment variable for our path.
    // If it isn't set, use the default path defined above.
    char *pathp;
    if ((pathp = getenv("PATH")) == NULL) {
        pathp = (char *) DEFAULT_PATH;
    }
    char **path = tokenize(pathp, ":", "");

    // Should this shell be interactive?
    bool interactive = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);

    // Main loop: print prompt, read line, execute command
    while (1) {
        // If `stdout' is a terminal (i.e., we're an interactive shell),
        // print a prompt before reading a line of input.
        if (interactive) {
            fputs(INTERACTIVE_PROMPT, stdout);
            fflush(stdout);
        }

        char line[MAX_LINE_CHARS];
        if (fgets(line, MAX_LINE_CHARS, stdin) == NULL)
            break;

        // Tokenise and execute the input line.
        char **command_words =
            tokenize(line, (char *) WORD_SEPARATORS, (char *) SPECIAL_CHARS);
        execute_command(command_words, path, environ);
        free_tokens(command_words);
    }

    free_tokens(path);
    return 0;
}


//
// Execute a command, and wait until it finishes.
//
//  * `words': a NULL-terminated array of words from the input command line
//  * `path': a NULL-terminated array of directories to search in;
//  * `environment': a NULL-terminated array of environment variables.
//
static void execute_command(char **words, char **path, char **environment)
{
    assert(words != NULL);
    assert(path != NULL);
    assert(environment != NULL);

    char *program = words[0];

    if (program == NULL) {
        // nothing to do
        return;
    }

    if (strcmp(program, "exit") == 0) {
        do_exit(words);
        // `do_exit' will only return if there was an error.
        return;
    }

    //////////////
    // SUBSET 2 //
    //////////////

    // Adapted from Tutorial 8 Q12
    char *home_pathname = getenv("HOME");
    if (home_pathname == NULL) {
        perror("HOME");
        return;
    }

    // Allows us to append each command to $HOME/.shuck_history
    char *file_name = ".shuck_history";
    int file_len = strlen(home_pathname) + strlen(file_name) + 2;
    char file_array[file_len];
    snprintf(file_array, sizeof file_array, "%s/%s", home_pathname, file_name);

    // If "!" is entered, print out the nth command and execute
    if (strcmp(program, "!") == 0) {
        int number = find_len(file_array);
        if (words[1] != NULL) {
            // Error Handling
            if (words[2] != NULL) {
                fprintf(stderr, "!: too many arguments\n");
                return;
            }
            char *endptr;
            number = (int)strtol(words[1], &endptr, 10);
            while (*endptr != '\0') {
                if (*endptr < '0' || *endptr > '9') {
                    fprintf(stderr, "!: %s: numeric argument required", words[1]);
                    return;
                } 
                endptr++;
            }
        } 
        print_exclam(number, file_array, path, environment);
        return;
    }     
    // Append any command into ./shuck_history if "!" is not entered
    else {
        FILE *f = fopen(file_array, "a");
        // Append any arguments to the .shuck_history file
        for (int i = 0; words[i] != NULL; i++) {
            fprintf(f, "%s ", words[i]);
        }
        fprintf(f, "\n");
        fclose(f);
    }


    //////////////
    // SUBSET 3 //
    //////////////

    // Calls globbing function on words for patterns (*,?,[,~)
    // If no match -> execute pattern normally
    words = expand_filename(words);

    ////////////////////////////
    // SUBSET 2 - continued ///
    ///////////////////////////

    // Prints out history per nth line
    if (strcmp(program, "history") == 0) {
        int num = DEFAULT_HISTORY_SHOWN;
        if (words[1] != NULL) {
            // Error Handling
            if (words[2] != NULL) {
                fprintf(stderr, "%s: too many arguments\n", program);
                return;
            }
            char *endptr;
            num = (int)strtol(words[1], &endptr, 10);
            // Check whether numeric argument has been entered in command
            while (*endptr != '\0') {
                if (*endptr < '0' || *endptr > '9') {
                    fprintf(stderr, "%s: nonnumber: numeric argument required\n", program);
                    return;
                } 
                endptr++;
            }
        }
        print_history(num, file_array);
        return;
    }

    ///////////////
    //  SUBSET 0 //
    ///////////////
    
    // cd -> change directory
    if (strcmp(program, "cd") == 0) {
        char *home_path = getenv("HOME");
        // temp will store the cd command
        char *temp = words[1];
        if (temp == NULL) {
            temp = home_path;
        }
        // adapted from recommended lecture example my_cd.c
        if (chdir(temp) != 0) {
            fprintf(stderr, "cd: %s: No such file or directory\n", temp);
            return;
        }
        return;
    }

    // pwd -> prints current directory
    if (strcmp(program, "pwd") == 0) {
        // adapted from recommended lecture example -> getcwd.c
        char pathname[PATH_MAX];
        if (getcwd(pathname, sizeof pathname) == NULL) {
            perror("getcwd");
            return;
        }
        // adapted from recommended lecture example -> get_status.c
        printf("current directory is '%s'\n", pathname);
        return;
    }

    ///////////////
    //  SUBSET 1 //
    ///////////////

    if (strrchr(program, '/') == 0) {
        int found = 0;
        for (int i = 0; path[i] != NULL; i++) {
            // Adapted from tutorial 8 - Q12 & spawn.c from lecture example
            int len = strlen(program) + strlen(path[i]) + 2;
            char pathname[len];
            snprintf(pathname, sizeof pathname, "%s/%s", path[i], program);
            // Check if the pathname can be executed
            found = is_executable(pathname);
            if (found == 1) {
                posix_helper(words, pathname, environment);
                break;
            } 
        }
        // If command is unable to be executed, fprintf error statement
        if (found == 0) {
            fprintf(stderr, "%s: command not found\n", program);
            return;
        }
    } else {
        // Check if program can be executed
        if (is_executable(program)) {
            posix_helper(words, program, environment);
        } else {
            // If command is unable to be executed, fprintf error statement
            fprintf(stderr, "%s: command not found\n", program);
        }
        return;
    }
}

/////////////////////
// HELPER FUNCTIONS//
/////////////////////

// Runs the posix_spawn -> adapted from spawn.c lecture example
void posix_helper(char **words, char *pathname, char **environment) {
    pid_t pid;
    if (posix_spawn(&pid, pathname, NULL, NULL, words, environment) != 0) {
        perror("spawn");
        return;
    }
    int exit_status;
    if (waitpid(pid, &exit_status, 0) == -1) {
        perror("waitpid");
        return;
    }
    // WIFEEXITED -> exited normally
    // WEXITSTATUS returns when child exists
    // https://www.geeksforgeeks.org/wait-system-call-c/
    printf("%s exit status = %d\n", pathname, WEXITSTATUS(exit_status));
    return;
}

// Runs through and prints the history as per the last n commands
// If n is not specified - automatically prints 10
void print_history(int num, char *pathname) {
    FILE *f = fopen(pathname, "r");
    if (f == NULL) {
        perror(pathname);
        return;
    }
    // Finds how many lines/commands are within history file
    int size = find_len(pathname);
    char array_len[MAX_LINE_CHARS];
    int i = 0;
    while (i < size && fgets(array_len, MAX_LINE_CHARS, f) != NULL) {
        if (i >= (size - num)) {
            printf("%d: %s", i, array_len);
        }
        i++;
    }
    fclose(f);
    return;
}

// Finds the length of the pathname (ie. how many lines within the file)
static int find_len(char *pathname) {
    int size = 0;
    FILE *f = fopen(pathname, "r");
    if (f == NULL) {
        perror(pathname);
        return 0;
    }
    char array_len[MAX_LINE_CHARS];
    while (fgets(array_len, MAX_LINE_CHARS, f) != NULL) size++;
    
    // Since index starts from 0
    size--;
    fclose(f);
    return size;
}

// Prints the nth command
// If n is not specified, it will print the last command and execute it
void print_exclam(int number, char *pathname, char **path, char **environment) {
    FILE *f = fopen(pathname, "r");
    // Finds how many lines/commands within history file
    int size = find_len(pathname);

    char array_len[MAX_LINE_CHARS];
    int i = 0;
    while (i <= size && fgets(array_len, MAX_LINE_CHARS, f) != NULL ) {
        if (i == number) {
            printf("%s", array_len);
            // Simliar to execution within main function
            char **words_temp = tokenize(array_len, (char *) WORD_SEPARATORS, (char *) SPECIAL_CHARS);
            execute_command(words_temp, path, environment);
            free_tokens(words_temp);
        } 
        i++;
    }
    fclose(f);
    return;
}

// Supports filename expansion - globbing
// Adapted from provided lecture example - glob.c
char **expand_filename(char **words) {
    assert(words != NULL);
    // Holds pattern expansion
    glob_t matches;
    // Glob the first command in the command line
    if (words[0] != NULL) {
        glob(words[0], GLOB_NOCHECK|GLOB_TILDE, NULL, &matches);
    }
    int i = 1;
    // Added GLOB_APPEND to append the pathnames generated
    while (words[i] != NULL) {
        glob(words[i], GLOB_NOCHECK|GLOB_TILDE|GLOB_APPEND, NULL, &matches);
        i++;
    }
    words = malloc((matches.gl_pathc + 1) * sizeof(char *));
    // Add to words array
    int j = 0;
    while (j < matches.gl_pathc) {
        // Returns pointer to new string - duplicates
        words[j] = strdup(matches.gl_pathv[j]);
        j++;
    }
    // Make last value in array NULL
    words[j] = NULL;
    globfree(&matches);
    return words;
}

//
// Implement the `exit' shell built-in, which exits the shell.
//
// Synopsis: exit [exit-status]
// Examples:
//     % exit
//     % exit 1
//
static void do_exit(char **words)
{
    assert(words != NULL);
    assert(strcmp(words[0], "exit") == 0);

    int exit_status = 0;

    if (words[1] != NULL && words[2] != NULL) {
        // { "exit", "word", "word", ... }
        fprintf(stderr, "exit: too many arguments\n");

    } else if (words[1] != NULL) {
        // { "exit", something, NULL }
        char *endptr;
        exit_status = (int) strtol(words[1], &endptr, 10);
        if (*endptr != '\0') {
            fprintf(stderr, "exit: %s: numeric argument required\n", words[1]);
        }
    }

    exit(exit_status);
}


//
// Check whether this process can execute a file.  This function will be
// useful while searching through the list of directories in the path to
// find an executable file.
//
static int is_executable(char *pathname)
{
    struct stat s;
    return
        // does the file exist?
        stat(pathname, &s) == 0 &&
        // is the file a regular file?
        S_ISREG(s.st_mode) &&
        // can we execute it?
        faccessat(AT_FDCWD, pathname, X_OK, AT_EACCESS) == 0;
}


//
// Split a string 's' into pieces by any one of a set of separators.
//
// Returns an array of strings, with the last element being `NULL'.
// The array itself, and the strings, are allocated with `malloc(3)';
// the provided `free_token' function can deallocate this.
//
static char **tokenize(char *s, char *separators, char *special_chars)
{
    size_t n_tokens = 0;

    // Allocate space for tokens.  We don't know how many tokens there
    // are yet --- pessimistically assume that every single character
    // will turn into a token.  (We fix this later.)
    char **tokens = calloc((strlen(s) + 1), sizeof *tokens);
    assert(tokens != NULL);

    while (*s != '\0') {
        // We are pointing at zero or more of any of the separators.
        // Skip all leading instances of the separators.
        s += strspn(s, separators);

        // Trailing separators after the last token mean that, at this
        // point, we are looking at the end of the string, so:
        if (*s == '\0') {
            break;
        }

        // Now, `s' points at one or more characters we want to keep.
        // The number of non-separator characters is the token length.
        size_t length = strcspn(s, separators);
        size_t length_without_specials = strcspn(s, special_chars);
        if (length_without_specials == 0) {
            length_without_specials = 1;
        }
        if (length_without_specials < length) {
            length = length_without_specials;
        }

        // Allocate a copy of the token.
        char *token = strndup(s, length);
        assert(token != NULL);
        s += length;

        // Add this token.
        tokens[n_tokens] = token;
        n_tokens++;
    }

    // Add the final `NULL'.
    tokens[n_tokens] = NULL;

    // Finally, shrink our array back down to the correct size.
    tokens = realloc(tokens, (n_tokens + 1) * sizeof *tokens);
    assert(tokens != NULL);

    return tokens;
}


//
// Free an array of strings as returned by `tokenize'.
//
static void free_tokens(char **tokens)
{
    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }
    free(tokens);
}
