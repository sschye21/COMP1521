#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shuck.h"
#include "print.h"

#define MAX_CHAR 2048

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
    char array_len[MAX_CHAR];
    int i = 0;
    while (i < size && fgets(array_len, MAX_CHAR, f) != NULL) {
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
    char array_len[MAX_CHAR];
    while (fgets(array_len, MAX_CHAR, f) != NULL) size++;
    
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

    char array_len[MAX_CHAR];
    int i = 0;
    while (i <= size && fgets(array_len, MAX_CHAR, f) != NULL ) {
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