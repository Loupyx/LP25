#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ui/key_detector.h"
#include "network/network_SSH.h"
#include "process/Processus.h"
#include "tool/tool.h"

/*fonction main */
int main() {

    char *test = get_char_file("/proc/95/stat");
    printf("%s\n", test);
    return 0;
}