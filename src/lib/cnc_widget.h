#ifndef CNC_WIDGETS_H
#define CNC_WIDGETS_H

// using cw as shorthand for cnc_widgets

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "cnc_buffer.h"

// max memory sizes
#define INFO_BUFFER_SIZE    511
#define PROMPT_BUFFER_SIZE  511
#define DISPLAY_BUFFER_SIZE 99999

// prompt symbols
#define PROMPT_PAD 3
#define PROMPT_INS ">> " // 3 bytes
#define PROMPT_CMD "_? " // 3 bytes

typedef enum
{
  WIDGET_TITLE,
  WIDGET_DISPLAY,
  WIDGET_INFO,
  WIDGET_PROMPT,

} cw_type;

typedef struct
{
  size_t row;
  size_t col;

} cnc_point;

typedef struct
{
  cnc_point origin;
  size_t    width;
  size_t    height;

} cnc_rect;

typedef struct
{
  cnc_rect frame;
  cw_type  type;

  // index: index of first visible element
  size_t index;

  // data_index: index of cursor in the data
  size_t data_index;

  cnc_buffer buffer;

  // info and prompt have homogeneous bg and fg colors
  cnc_term_token bg;
  cnc_term_token fg;

  cnc_term_token bg_main;
  cnc_term_token fg_main;

  cnc_term_token bg_alt;
  cnc_term_token fg_alt;

  bool can_focus;
  bool has_focus;

} cnc_widget;

// main functions
void cw_destroy(cnc_widget **cw);

cnc_widget *cw_init(cw_type type);

void cw_reset(cnc_widget *cw);

#endif
