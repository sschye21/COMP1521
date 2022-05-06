// Runs through and prints the history as per the last n commands
// If n is not specified - automatically prints 10
void print_history(int num, char *pathname);

// Finds the length of the pathname (ie. how many lines within the file)
int find_len(char *pathname);

// Prints the nth command
// If n is not specified, it will print the last command
void print_exclam(int number, char *pathname, char **path, char **environment);