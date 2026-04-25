/*
 * DevilCore OS — DevilShell (dsh)
 */
#ifndef DC_DSH_H
#define DC_DSH_H

#include "../include/types.h"
#include "../gui/wm.h"

/* Pass an input command string to the shell for execution */
void dsh_execute(Window *term_win, const char *command);

#endif /* DC_DSH_H */
