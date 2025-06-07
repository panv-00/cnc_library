#ifndef CNC_TERMINAL_H
#define CNC_TERMINAL_H

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
  cnc_widget    *main_display_widget;
  ct_row_info   *rows_info;
  cnc_cursor     cursor;

} cnc_terminal;

typedef void (*ActionFunc)(cnc_terminal *ct);

typedef struct
{
  int        key;
  ActionFunc func;

} CommandMap;

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
