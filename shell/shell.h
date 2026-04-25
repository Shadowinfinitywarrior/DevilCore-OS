#ifndef _SHELL_H
#define _SHELL_H

#include <devil/types.h>

void shell_init(void);
void shell_prompt(void);
int shell_parse(const char *cmd);
int shell_execute(const char *cmd, char *const argv[]);

#endif // _SHELL_H