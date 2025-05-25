#ifndef CNC_TERMINAL_H
#define CNC_TERMINAL_H

// #define _GNU_SOURCE

// using ct as shorthand for cnc_terminal

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "cnc_buffer.h"
#include "cnc_cursor.h"
#include "cnc_widget.h"

// Cursor and Screen Stuff
#define CLRSCR        write(STDOUT_FILENO, "\x1b[2J\x1b[3J", 8)
#define HOME_POSITION write(STDOUT_FILENO, "\x1b[H", 3)
#define HIDE_CURSOR   write(STDOUT_FILENO, "\x1b[?25l", 6)
#define SHOW_CURSOR   write(STDOUT_FILENO, "\x1b[?25h", 6)
#define CURSOR_INS    write(STDOUT_FILENO, "\x1b[5 q", 5)
#define CURSOR_CMD    write(STDOUT_FILENO, "\x1b[1 q", 5)
#define POSCURSOR(c, r)                                                        \
  do                                                                           \
  {                                                                            \
    write(STDOUT_FILENO, "\x1b[", 2);                                          \
    char __buf[43];                                                            \
    int  __len = sprintf(__buf, "%zu;%zuH", (size_t)(r), (size_t)(c));         \
    write(STDOUT_FILENO, __buf, __len);                                        \
  } while (0)

// Reset styles constant
#define STR_RESET_STYLES "\x1b[0m"

typedef enum
{
  MODE_CMD, // command mode
  MODE_INS  // insert mode

} ct_mode;

typedef struct
{
  size_t         operation;
  cnc_term_token value;

} ct_color;

typedef struct
{
  int8_t         is_valid;
  size_t         first_index; // display token index included
  size_t         last_index;  // display token index included
  cnc_term_token bg;          // row background color
  cnc_term_token fg;          // row foreground color

} ct_row_info;

typedef struct
{
  // signal handling struct
  struct sigaction sa_resize;
  struct sigaction sa_sigtstp;

  size_t         min_width;
  size_t         min_height;
  size_t         scr_rows;
  size_t         scr_cols;
  struct termios orig_term;
  bool           in_raw_mode;
  bool           can_change_mode;
  bool           can_change_focus;
  ct_mode        mode;
  char          *screenbuffer;
  size_t         screenbuffer_size;
  uint8_t        widgets_count;
  cnc_widget   **widgets;
  cnc_widget    *focused_widget;
  ct_row_info   *rows_info;
  cnc_cursor     cursor;

} cnc_terminal;

// main functions
cnc_widget *ct_add_widget(cnc_terminal *ct, cw_type type);

void ct_check_for_resize(cnc_terminal *ct);
void ct_destroy(cnc_terminal *ct);
void ct_focus_next(cnc_terminal *ct);
void ct_focus_widget(cnc_terminal *ct, cnc_widget *cw);

cnc_widget *ct_focused_widget(cnc_terminal *ct);

bool ct_get_size(cnc_terminal *ct);
int  ct_get_user_input(cnc_terminal *ct);

cnc_terminal *ct_init(size_t min_height, size_t min_width);

void ct_screenbuffer_reset(cnc_terminal *ct);
void ct_set_mode(cnc_terminal *ct, ct_mode mode);
bool ct_setup_widgets(cnc_terminal *ct);
void ct_update(cnc_terminal *ct);

#endif
