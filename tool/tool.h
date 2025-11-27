#ifndef TOOL_H
#define TOOL_H

#include "./../network/network_SSH.h"

char *get_char_file(char *path);

char *get_char_ssh(ssh_state *state, char *path);

#endif