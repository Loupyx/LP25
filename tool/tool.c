#include <stdlib.h>
#include <stdio.h>

#include "tool.h"

char *get_char_file(char *path) {
    FILE *file;
    char *text = NULL, c;
    int size = 0,ic;
    file = fopen(path, "r");
    if (!file) {
        printf("Error fopen\n");
        return NULL;
    }
    while ((ic = fgetc(file)) != EOF) {
        c = (char)ic;
        if (c != '\n') {
            char *temp = (char*)realloc(text, (size+1)*sizeof(char));
            if (!temp) {
                free(text);
                fclose(file);
                return NULL;
            }
            text = temp;
            text[size++] = c;
        }
    }

    if (!text) {
        text = malloc(1);
        if (!text) return NULL;
        text[0] = '\0';
        return text;
    }
    
    text[size] = '\0';
    return text;
}

