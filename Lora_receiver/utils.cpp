#include "utils.h"
char** splitString(const char* input, int* numTokens) {
    char** tokens = NULL;
    char* token = NULL;
    int tokenCount = 0;
    char inputCopy[MAX_SIZE];
    strcpy(inputCopy, input);

    token = strtok(inputCopy, ",");
    while (token != NULL) {
        tokens = (char**)realloc(tokens, (tokenCount + 1) * sizeof(char*));
        tokens[tokenCount] = (char*)malloc((strlen(token) + 1) * sizeof(char));
        strcpy(tokens[tokenCount], token);
        tokenCount++;

        token = strtok(NULL, ", ");
    }

    *numTokens = tokenCount;
    return tokens;
}