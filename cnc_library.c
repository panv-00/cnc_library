#include "cnc_library.h"

// Signals

// suspend flag
volatile sig_atomic_t suspend_flag = 0;
static void _handle_sigtstp(int sig) { suspend_flag = 1; }

// resize flag
volatile sig_atomic_t resize_flag = 0;
static void _handle_resize(int sig) { resize_flag = 1; }

/*** A. Static Functions Declaration ***/

static void _cnc_terminal_check_for_suspend(cnc_terminal *t);

static bool _cnc_terminal_set_raw_mode(cnc_terminal *t);
static void _cnc_terminal_restore(cnc_terminal *t);

// following function returns 1 for bg color and 2 for fg color
// it ultimitely sets the color in char *color
// this function will return 0 when resetting colors
static int _color_code_to_color(int color_code, char **color);

// Vim-Like functions
static void _vim_mode_k(cnc_widget *w);
static void _vim_mode_j(cnc_widget *w);
static void _vim_mode_l(cnc_widget *w);
static void _vim_mode_h(cnc_widget *w);
static void _vim_mode_e(cnc_widget *w);
static void _vim_mode_b(cnc_widget *w);
static void _vim_mode_0(cnc_widget *w);
static void _vim_mode_$(cnc_widget *w);
static void _vim_mode_x(cnc_widget *w);

// page_up | page_down
static void _page_up(cnc_widget *w);
static void _page_dn(cnc_widget *w);
static void _insert_char(cnc_widget *w, char c);
static void _delete_char(cnc_widget *w);

// screen buffer index calculator
static size_t _index_at_cr(cnc_terminal *t, size_t c, size_t r);

// get user input
static int _cnc_terminal_getch(cnc_terminal *t);
static int _cnc_terminal_get_user_input(cnc_terminal *t);

/*** B. Static Functions Definition ***/

static void _cnc_terminal_check_for_suspend(cnc_terminal *t)
{
  if (suspend_flag)
  {
    suspend_flag = 0;

    _cnc_terminal_restore(t);

    // Set default handler for SIGTSTP
    signal(SIGTSTP, SIG_DFL);

    // Send SIGTSTP to itself to suspend the process
    raise(SIGTSTP);
  }

  else
  {
    if (t->in_raw_mode == false)
    {
      _cnc_terminal_set_raw_mode(t);
      cnc_terminal_update_and_redraw(t);
    }
  }
}

static bool _cnc_terminal_set_raw_mode(cnc_terminal *t)
{
  CLRSCR;
  HOME_POSITION;

  // read the current terminal attributes and store them
  if (tcgetattr(STDIN_FILENO, &t->orig_term) == -1)
  {
    return false;
  }

  // put the terminal in raw mode
  struct termios raw_term = t->orig_term;

  raw_term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw_term.c_oflag &= ~(OPOST);
  raw_term.c_cflag |= (CS8);
  raw_term.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw_term.c_cc[VMIN]  = 0;
  raw_term.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_term) == -1)
  {
    return false;
  }

  t->in_raw_mode = true;

  return true;
}

static void _cnc_terminal_restore(cnc_terminal *t)
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &t->orig_term);
  t->in_raw_mode = false;

  // Restore cursor
  CURSOR_INS;
  SHOW_CURSOR;

  // Restore color
  write(STDOUT_FILENO, COLOR_DEFAULT, 5);

  // Clear screen
  CLRSCR;
  HOME_POSITION;
  fflush(stdout);
}

/*** 1. COLORS ***/
static int _color_code_to_color(int color_code, char **color)
{
  switch (color_code)
  {
  case COLOR_CODE_BLACK_FG:
    *color = COLOR_BLACK_FG;
    return 2;

  case COLOR_CODE_RED_FG:
    *color = COLOR_RED_FG;
    return 2;

  case COLOR_CODE_GREEN_FG:
    *color = COLOR_GREEN_FG;
    return 2;

  case COLOR_CODE_YELLOW_FG:
    *color = COLOR_YELLOW_FG;
    return 2;

  case COLOR_CODE_BLUE_FG:
    *color = COLOR_BLUE_FG;
    return 2;

  case COLOR_CODE_MAGENTA_FG:
    *color = COLOR_MAGENTA_FG;
    return 2;

  case COLOR_CODE_CYAN_FG:
    *color = COLOR_CYAN_FG;
    return 2;

  case COLOR_CODE_WHITE_FG:
    *color = COLOR_WHITE_FG;
    return 2;

  case COLOR_CODE_BLACK_BG:
    *color = COLOR_BLACK_BG;
    return 1;

  case COLOR_CODE_RED_BG:
    *color = COLOR_RED_BG;
    return 1;

  case COLOR_CODE_GREEN_BG:
    *color = COLOR_GREEN_BG;
    return 1;

  case COLOR_CODE_YELLOW_BG:
    *color = COLOR_YELLOW_BG;
    return 1;

  case COLOR_CODE_BLUE_BG:
    *color = COLOR_BLUE_BG;
    return 1;

  case COLOR_CODE_MAGENTA_BG:
    *color = COLOR_MAGENTA_BG;
    return 1;

  case COLOR_CODE_CYAN_BG:
    *color = COLOR_CYAN_BG;
    return 1;

  case COLOR_CODE_WHITE_BG:
    *color = COLOR_WHITE_BG;
    return 1;

  default:
    return 0;
  }
}

/*** 2. BUFFER ***/
// char array length function
size_t calen(const char *str)
{
  if (!str)
  {
    return 0;
  }

  const char *s;

  for (s = str; *s; ++s)
  {
    // just loop
  }

  return (s - str);
}

cnc_buffer *cnc_buffer_init(size_t size)
{
  cnc_buffer *b = (cnc_buffer *)malloc(sizeof(cnc_buffer));

  if (!b)
  {
    return NULL;
  }

  b->size     = size;
  b->length   = 0;
  b->contents = (char *)malloc((size + 1) * sizeof(char));

  if (!b->contents)
  {
    cnc_buffer_destroy(b);
    return NULL;
  }

  for (size_t i = 0; i <= size; i++)
  {
    b->contents[i] = '\0';
  }

  return b;
}

cnc_buffer *cnc_buffer_resize(cnc_buffer *b, size_t size)
{
  b->size     = size;
  b->length   = 0;
  b->contents = (char *)realloc(b->contents, (size + 1) * sizeof(char));

  if (!b->contents)
  {
    cnc_buffer_destroy(b);
    return NULL;
  }

  for (size_t i = 0; i <= size; i++)
  {
    b->contents[i] = '\0';
  }

  return b;
}

void cnc_buffer_set_text(cnc_buffer *b, const char *text)
{
  b->length = calen(text);

  if (b->length > b->size)
  {
    b->length = b->size;
  }

  for (size_t i = 0; i <= b->size; i++)
  {
    if (i < b->length)
    {
      b->contents[i] = text[i];
    }

    else
    {
      b->contents[i] = '\0';
    }
  }
}

void cnc_buffer_replace(cnc_buffer *b, const char orig, const char dest)
{
  for (size_t i = 0; i < b->length; i++)
  {
    if (b->contents[i] == orig)
    {
      b->contents[i] = dest;
    }
  }
}

void cnc_buffer_replace_text(cnc_buffer *b, size_t start, size_t len,
                             const char *text, size_t text_start)
{
  if (start > b->length)
  {
    return;
  }

  size_t text_length = calen(text);
  size_t length      = len;

  if (text_length - text_start < len)
  {
    length = text_length - text_start;
  }

  for (size_t i = start; i < start + length; i++)
  {
    if (i >= b->size)
    {
      return;
    }

    if (i - start + text_start < text_length)
    {
      b->contents[i] = text[i - start + text_start];

      if (i >= b->length)
      {
        b->length++;
      }
    }

    else
    {
      break;
    }
  }
}

void cnc_buffer_replace_char(cnc_buffer *b, size_t start, size_t len, char c)
{
  if (start > b->length)
  {
    return;
  }

  for (size_t i = start; i < start + len; i++)
  {
    if (i >= b->length)
    {
      return;
    }

    b->contents[i] = c;
  }
}

size_t cnc_buffer_insert_text(cnc_buffer *b, size_t start, size_t len,
                              const char *text)
{
  if (start >= b->size)
  {
    return 0;
  }

  // start at the end if start is bigger than b->length
  size_t actual_start = start > b->length ? b->length : start;

  // calculate actual_length
  size_t text_length   = calen(text);
  size_t actual_length = len > text_length ? text_length : len;

  if (b->length + actual_length > b->size)
  {
    actual_length = b->size - b->length;
  }

  // push existing memory at actual_start an amount of actual_length
  for (size_t i = b->length + actual_length - 1;
       i > actual_start + actual_length - 1; i--)
  {
    b->contents[i] = b->contents[i - actual_length];
  }

  // write the memory as required
  for (size_t i = actual_start; i < actual_start + actual_length; i++)
  {
    b->contents[i] = text[i - actual_start];
  }

  b->length              = b->length + actual_length;
  b->contents[b->length] = '\0';

  return actual_length;
}

size_t cnc_buffer_append(cnc_buffer *b, const char *text)
{
  return cnc_buffer_insert_text(b, b->length, calen(text), text);
}

size_t cnc_buffer_insert_char(cnc_buffer *b, size_t start, size_t len, char c)
{
  if (start >= b->size)
  {
    return 0;
  }

  // start at the end if start is bigger than b->length
  size_t actual_start = start > b->length ? b->length : start;

  // calculate actual_length
  size_t actual_length = b->length + len > b->size ? b->size - b->length : len;

  // push existing memory at actual_start an amount of actual_length
  for (size_t i = b->length + actual_length - 1;
       i > actual_start + actual_length - 1; i--)
  {
    b->contents[i] = b->contents[i - actual_length];
  }

  // write the memory as required
  for (size_t i = actual_start; i < actual_start + actual_length; i++)
  {
    b->contents[i] = c;
  }

  b->length              = b->length + actual_length;
  b->contents[b->length] = '\0';

  return actual_length;
}

bool cnc_buffer_delete_char(cnc_buffer *b, size_t location)
{
  if (b->length == 0 || location > b->length)
  {
    return false;
  }

  for (size_t i = location; i < b->length; i++)
  {
    b->contents[i] = b->contents[i + 1];
  }

  b->contents[b->length--] = '\0';

  return true;
}

bool cnc_buffer_equal_string(cnc_buffer *b, const char *str)
{
  if (b->length != calen(b->contents) || b->length != calen(str))
  {
    return false;
  }

  for (size_t i = 0; i < b->length; i++)
  {
    if (b->contents[i] != str[i])
    {
      return false;
    }
  }

  return true;
}

bool cnc_buffer_locate_string(cnc_buffer *b, const char *s, size_t *location)
{
  if (b == NULL || b->contents == NULL || s == NULL || location == NULL)
  {
    return false;
  }

  size_t buffer_length = b->length;
  size_t str_length    = calen(s);

  if (buffer_length == 0 || buffer_length < str_length)
  {
    return false;
  }

  if (str_length == 0)
  {
    *location = 0;
    return true;
  }

  for (size_t i = 0; i <= buffer_length - str_length; i++)
  {
    size_t j;

    for (j = 0; j < str_length; j++)
    {
      if (b->contents[i + j] != s[j])
      {
        break;
      }
    }

    if (j == str_length)
    {
      *location = i;
      return true;
    }
  }

  return false;
}

void cnc_buffer_trim(cnc_buffer *b)
{
  if (!b)
  {
    return;
  }

  cnc_buffer *temp_buffer = cnc_buffer_init(b->size);

  size_t start = 0;
  size_t end   = 0;

  while (b->contents[start] == ' ')
  {
    start++;
  }

  while (b->contents[end] != '\0')
  {
    end++;
  }

  end--;

  while (b->contents[end] == ' ')
  {
    end--;
  }

  end++;

  if (end > start)
  {
    cnc_buffer_replace_text(temp_buffer, 0, end - start, b->contents, start);
    cnc_buffer_set_text(b, temp_buffer->contents);
  }

  else
  {
    cnc_buffer_clear(b);
  }

  cnc_buffer_destroy(temp_buffer);
}

void cnc_buffer_clear(cnc_buffer *b)
{
  if (!b)
  {
    return;
  }

  for (size_t i = 0; i < b->size; i++)
  {
    b->contents[i] = '\0';
  }

  b->length = 0;
}

void cnc_buffer_destroy(cnc_buffer *b)
{
  if (!b)
  {
    return;
  }

  if (b->contents)
  {
    free(b->contents);
    b->contents = NULL;
  }

  free(b);
  b = NULL;
}

/*** 3. WIDGETS ***/

cnc_widget *cnc_widget_init(cnc_widget_type type)
{
  cnc_widget *w      = (cnc_widget *)malloc(sizeof(cnc_widget));
  size_t buffer_size = 0;

  if (!w)
  {
    return NULL;
  }

  w->frame.origin.col = 1;
  w->frame.origin.row = 1;
  w->frame.height     = 2;
  w->frame.width      = 1;

  w->type = type;

  w->index      = 0;
  w->data_index = 0;
  w->data       = NULL;

  w->background = NULL;
  w->foreground = NULL;

  w->has_focus = false;

  switch (type)
  {
  case WIDGET_INFO:
    buffer_size   = INFO_BUFFER_SIZE;
    w->background = COLOR_GREEN_BG;
    w->foreground = COLOR_BLACK_FG;
    w->can_focus  = false;
    break;

  case WIDGET_PROMPT:
    buffer_size   = PROMPT_BUFFER_SIZE;
    w->foreground = COLOR_CYAN_FG;
    w->can_focus  = true;
    break;

  case WIDGET_DISPLAY:
    buffer_size  = DISPLAY_BUFFER_SIZE;
    w->can_focus = true;
    break;
  }

  w->data = cnc_buffer_init(buffer_size);

  if (!w->data)
  {
    cnc_widget_destroy(w);
    return NULL;
  }

  return w;
}

void cnc_widget_reset(cnc_widget *w)
{
  cnc_buffer_set_text(w->data, "");
  w->data_index = 0;
  w->index      = 0;
}

void cnc_widget_destroy(cnc_widget *w)
{
  if (!w)
  {
    return;
  }

  if (w->data)
  {
    cnc_buffer_destroy(w->data);
  }

  free(w);
  w = NULL;
}

/*** 4. TERMINAL ***/

// Position Cursor function
void POSCURSOR(size_t c, size_t r)
{
  write(STDOUT_FILENO, "\x1b[", 2);
  char buf[43];
  int len = sprintf(buf, "%zu;%zuH", r, c);
  write(STDOUT_FILENO, buf, len);
}

// Vim-Like functions
static void _vim_mode_k(cnc_widget *w)
{
  if (w && w->type == WIDGET_DISPLAY &&
      w->index + w->frame.height < w->data_index)
  {
    w->index++;
  }
}

static void _vim_mode_j(cnc_widget *w)
{
  if (w && w->type == WIDGET_DISPLAY && w->index > 0)
  {
    w->index--;
  }
}

static void _vim_mode_l(cnc_widget *w)
{
  if (w && w->type == WIDGET_PROMPT && w->data_index < w->data->length)
  {
    w->data_index++;

    if (w->data_index - w->index > w->frame.width - 3)
    {
      w->index++;
    }
  }
}

static void _vim_mode_h(cnc_widget *w)
{
  if (w && w->type == WIDGET_PROMPT && w->data_index > 0)
  {
    w->data_index--;

    if (w->index > 0 && w->data_index - w->index < w->frame.width - 6)
    {
      w->index--;
    }
  }
}

static void _vim_mode_e(cnc_widget *w)
{
  bool move_forward       = w->data_index < w->data->length;
  bool curr_char_is_space = w->data->contents[w->data_index] == ' ';

  if (move_forward)
  {
    _vim_mode_l(w);
    move_forward       = w->data_index < w->data->length;
    curr_char_is_space = w->data->contents[w->data_index] == ' ';
  }

  while (move_forward && curr_char_is_space)
  {
    _vim_mode_l(w);
    move_forward       = w->data_index < w->data->length;
    curr_char_is_space = w->data->contents[w->data_index] == ' ';
  }

  while (move_forward && !curr_char_is_space)
  {
    _vim_mode_l(w);
    move_forward       = w->data_index < w->data->length;
    curr_char_is_space = w->data->contents[w->data_index] == ' ';
  }

  _vim_mode_h(w);
}

static void _vim_mode_b(cnc_widget *w)
{
  bool move_backward      = w->data_index > 0;
  bool curr_char_is_space = w->data->contents[w->data_index] == ' ';

  if (move_backward)
  {
    _vim_mode_h(w);
    move_backward      = w->data_index > 0;
    curr_char_is_space = w->data->contents[w->data_index] == ' ';
  }

  while (move_backward && curr_char_is_space)
  {
    _vim_mode_h(w);
    move_backward      = w->data_index > 0;
    curr_char_is_space = w->data->contents[w->data_index] == ' ';
  }

  while (move_backward && !curr_char_is_space)
  {
    _vim_mode_h(w);
    move_backward      = w->data_index > 0;
    curr_char_is_space = w->data->contents[w->data_index] == ' ';
  }

  if (w->data_index > 0)
  {
    _vim_mode_l(w);
  }
}

static void _vim_mode_0(cnc_widget *w)
{
  if (w && w->type == WIDGET_PROMPT)
  {
    w->data_index = 0;
    w->index      = 0;
  }

  if (w && w->type == WIDGET_DISPLAY)
  {
    if (w->data_index > w->frame.height)
    {
      w->index = w->data_index - w->frame.height;
    }
  }
}

static void _vim_mode_$(cnc_widget *w)
{
  if (w && w->type == WIDGET_PROMPT)
  {
    w->data_index = w->data->length;

    if (w->data_index > w->frame.width - 3)
    {
      w->index = w->data_index - w->frame.width + 3;
    }
  }

  if (w && w->type == WIDGET_DISPLAY)
  {
    w->index = 0;
  }
}

static void _vim_mode_x(cnc_widget *w)
{
  if (w && w->type == WIDGET_PROMPT && w->data_index < w->data->length)
  {
    cnc_buffer_delete_char(w->data, w->data_index);
  }
}

static void _page_up(cnc_widget *w)
{
  if (!w || w->type != WIDGET_DISPLAY)
  {
    return;
  }

  if (w->index + w->frame.height < w->data_index)
  {
    w->index += w->frame.height;
  }

  else if (w->data_index > w->frame.height)
  {
    w->index = w->data_index - w->frame.height;
  }
}

static void _page_dn(cnc_widget *w)
{
  if (w && w->type == WIDGET_DISPLAY && w->index > 0)
  {
    if (w->index > w->frame.height)
    {
      w->index -= w->frame.height;
    }

    else
    {
      w->index = 0;
    }
  }
}

static void _insert_char(cnc_widget *w, char c)
{
  if (w && w->type == WIDGET_PROMPT)
  {
    if (cnc_buffer_insert_char(w->data, w->data_index, 1, c))
    {
      w->data_index++;

      if (w->data_index > w->frame.width - 3)
      {
        w->index++;
      }
    }
  }
}

static void _delete_char(cnc_widget *w)
{
  if (w && w->type == WIDGET_PROMPT)
  {
    if (w->data_index > 0)
    {
      if (cnc_buffer_delete_char(w->data, w->data_index - 1))
      {
        w->data_index--;

        if (w->index > 0)
        {
          w->index--;
        }
      }
    }
  }
}

static size_t _index_at_cr(cnc_terminal *t, size_t c, size_t r)
{
  return (r - 1) * (t->scr_cols + 16) + c + 9;
}

void cnc_terminal_check_for_resize(cnc_terminal *t)
{
  if (resize_flag)
  {
    resize_flag = 0;
    if (!cnc_terminal_get_size(t))
    {
      return;
    }

    CLRSCR;
    HOME_POSITION;

    uint16_t new_rows = t->scr_rows;
    uint16_t new_cols = t->scr_cols;

    t->screen_buffer =
        cnc_buffer_resize(t->screen_buffer, (new_cols + 16) * new_rows);
    cnc_terminal_screenbuffer_reset(t);
    cnc_terminal_setup_widgets(t);
    cnc_terminal_update_and_redraw(t);
  }
}

cnc_terminal *cnc_terminal_init(size_t min_width, size_t min_height)
{
  cnc_terminal *t = (cnc_terminal *)malloc(sizeof(cnc_terminal));

  if (!t)
  {
    return NULL;
  }

  // Setup the SIGTSTP signal handler
  t->sa_sigtstp.sa_handler = _handle_sigtstp;
  t->sa_sigtstp.sa_flags   = SA_RESTART;
  sigemptyset(&t->sa_sigtstp.sa_mask);

  if (sigaction(SIGTSTP, &t->sa_sigtstp, NULL) == -1)
  {
    cnc_terminal_destroy(t);
    return NULL;
  }

  // Setup resize signal handling
  t->sa_resize.sa_handler = _handle_resize;
  t->sa_resize.sa_flags   = SA_RESTART;
  sigemptyset(&t->sa_resize.sa_mask);

  if (sigaction(SIGWINCH, &t->sa_resize, NULL) == -1)
  {
    cnc_terminal_destroy(t);
    return NULL;
  }

  // initialize terminal settings
  t->min_width  = min_width;
  t->min_height = min_height;

  t->scr_rows         = 0;
  t->scr_cols         = 0;
  t->in_raw_mode      = false;
  t->can_change_mode  = true;
  t->can_change_focus = true;
  t->widgets_count    = 0;
  t->cursor_col       = 1;
  t->cursor_row       = 1;

  t->widgets = (cnc_widget **)malloc(sizeof(cnc_widget *));

  if (!t->widgets)
  {
    cnc_terminal_destroy(t);
    return NULL;
  }

  // no widget has focus at the beginning
  t->focused_widget = NULL;

  if (_cnc_terminal_set_raw_mode(t) == false)
  {
    cnc_terminal_destroy(t);

    return NULL;
  }

  // get the terminal size
  if (!cnc_terminal_get_size(t))
  {
    cnc_terminal_destroy(t);

    return NULL;
  }

  // t->screen_buffer memory allocation:
  // number of rows: t->scr_rows.
  // number of cols: t->scr_cols.
  // number of bytes in each row:
  //  - color bg      : 5
  //  - color fg      : 5
  //  - total cols    : t->scr_cols
  //  - color reset   : 4
  //  - '\n\r'        : 2
  //
  // Total bytes in each line: t->scr_cols + 16

  t->screen_buffer = cnc_buffer_init((t->scr_cols + 16) * t->scr_rows);

  if (!t->screen_buffer)
  {
    cnc_terminal_destroy(t);

    return NULL;
  }

  // put terminal in command mode
  cnc_terminal_set_mode(t, MODE_CMD);

  // empty the buffer and redraw (clear the screen)
  cnc_terminal_screenbuffer_reset(t);
  cnc_terminal_redraw(t);

  return t;
}

bool cnc_terminal_get_size(cnc_terminal *t)
{
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0 ||
      ws.ws_row == 0)
  {
    return false;
  }

  t->scr_rows = ws.ws_row;
  t->scr_cols = ws.ws_col;

  return true;
}

void cnc_terminal_set_mode(cnc_terminal *t, cnc_terminal_mode mode)
{
  if (!t->can_change_mode)
  {
    return;
  }

  if (mode == MODE_CMD)
  {
    CURSOR_CMD;
  }

  else
  {
    CURSOR_INS;
  }

  t->mode = mode;
}

cnc_widget *cnc_terminal_add_widget(cnc_terminal *t, cnc_widget_type type)
{
  cnc_widget *w = cnc_widget_init(type);

  if (!w)
  {
    return NULL;
  }

  size_t height = 0;

  for (size_t i = 0; i < t->widgets_count; i++)
  {
    height += t->widgets[i]->frame.height;
  }

  if (w->frame.height + height > t->scr_rows)
  {
    cnc_widget_destroy(w);

    return NULL;
  }

  // resize the widgets array
  if (t->widgets_count > 0)
  {
    t->widgets = (cnc_widget **)realloc(t->widgets, (t->widgets_count + 1) *
                                                        sizeof(cnc_widget *));
  }

  if (!t->widgets)
  {
    cnc_widget_destroy(w);

    return NULL;
  }

  t->widgets[t->widgets_count++] = w;

  cnc_terminal_focus_widget(t, w);

  return w;
}

bool cnc_terminal_setup_widgets(cnc_terminal *t)
{
  size_t tlw        = 0; // number of two_lines_widgets
  size_t mlw        = 0; // number of multi_line_widgets
  size_t global_row = 1;

  for (size_t i = 0; i < t->widgets_count; i++)
  {
    switch (t->widgets[i]->type)
    {
    case WIDGET_INFO:
    case WIDGET_PROMPT:
      tlw++;
      break;

    case WIDGET_DISPLAY:
      mlw++;
      break;
    }
  }

  if (t->scr_rows < (2 * tlw + 3 * (mlw)))
  {
    return false;
  }

  // Calculate locations and dimensions
  size_t set_display_widget     = 1;
  size_t display_widgets_height = 0;

  for (size_t i = 0; i < t->widgets_count; i++)
  {
    // set correct width and resize widgets lines
    t->widgets[i]->frame.origin.row = global_row;

    if (t->widgets[i]->type == WIDGET_DISPLAY)
    {
      if (set_display_widget < mlw)
      {
        t->widgets[i]->frame.height = (t->scr_rows - 2 * tlw) / mlw;
        display_widgets_height += t->widgets[i]->frame.height;
        set_display_widget++;
      }

      else
      {
        t->widgets[i]->frame.height =
            t->scr_rows - 2 * tlw - display_widgets_height;
      }
    }

    t->widgets[i]->frame.width = t->scr_cols;
    global_row += t->widgets[i]->frame.height;
  }

  return true;
}

void cnc_terminal_screenbuffer_reset_char(cnc_terminal *t, char c)
{
  size_t sb_i = 0; // screen_buffer index

  cnc_buffer_set_text(t->screen_buffer, "");

  for (size_t i = 0; i < t->scr_rows; i++)
  {
    // number of bytes in each row:
    //  - color bg      : 5
    //  - color fg      : 5
    //  - total cols    : t->scr_cols
    //  - color reset   : 4
    //  - '\n\r'        : 2
    cnc_buffer_insert_text(t->screen_buffer, sb_i, 5, COLOR_NONE);
    cnc_buffer_insert_text(t->screen_buffer, sb_i + 5, 5, COLOR_NONE);
    cnc_buffer_insert_char(t->screen_buffer, sb_i + 10, t->scr_cols, c);
    cnc_buffer_insert_text(t->screen_buffer, sb_i + 10 + t->scr_cols, 4,
                           COLOR_DEFAULT);

    if (i != t->scr_rows - 1)
    {
      cnc_buffer_insert_char(t->screen_buffer, sb_i + 14 + t->scr_cols, 1,
                             '\n');
      cnc_buffer_insert_char(t->screen_buffer, sb_i + 15 + t->scr_cols, 1,
                             '\r');
    }

    sb_i = sb_i + 16 + t->scr_cols;
  }
}

void cnc_terminal_screenbuffer_reset(cnc_terminal *t)
{
  cnc_terminal_screenbuffer_reset_char(t, ' ');
}

void cnc_terminal_focus_widget(cnc_terminal *t, cnc_widget *w)
{
  if (!w->can_focus || w->has_focus)
  {
    return;
  }

  for (size_t i = 0; i < t->widgets_count; i++)
  {
    if (t->widgets[i] != w && t->widgets[i]->has_focus)
    {
      t->widgets[i]->has_focus = false;
      break;
    }
  }

  w->has_focus      = true;
  t->focused_widget = w;
}

cnc_widget *cnc_terminal_focused_widget(cnc_terminal *t)
{
  for (size_t i = 0; i < t->widgets_count; i++)
  {
    if (t->widgets[i]->has_focus)
    {
      return t->widgets[i];
    }
  }

  return NULL;
}

void cnc_terminal_focus_next(cnc_terminal *t)
{
  if (!t->can_change_focus)
  {
    return;
  }

  int focused_index = -1;

  for (size_t i = 0; i < t->widgets_count; i++)
  {
    if (t->widgets[i]->has_focus)
    {
      focused_index = i;
      break;
    }
  }

  for (size_t i = 1; i <= t->widgets_count; i++)
  {
    size_t index = (focused_index + i) % t->widgets_count;

    if (t->widgets[index]->can_focus)
    {
      cnc_terminal_focus_widget(t, t->widgets[index]);
      break;
    }
  }
}

void cnc_terminal_redraw(cnc_terminal *t)
{
  HIDE_CURSOR;
  HOME_POSITION;
  CLRSCR;

  // check if terminal dimensions are within limits
  if (t->scr_cols < t->min_width || t->scr_rows < t->min_height)
  {
    printf(".. Please resize your terminal\n\r");
    printf(".. Terminal Size: R=%4zu, C=%4zu\n\r", t->scr_rows, t->scr_cols);
    printf("..      Min Size: R=%4zu, C=%4zu\n\r", t->min_height, t->min_width);

    fflush(stdout);
    return;
  }

  write(STDOUT_FILENO, t->screen_buffer->contents, t->screen_buffer->size);

  cnc_widget *w = cnc_terminal_focused_widget(t);

  if (w && w->type == WIDGET_PROMPT)
  {
    t->cursor_col = w->data_index - w->index + 3;
    t->cursor_row = w->frame.origin.row + 1;
    POSCURSOR(t->cursor_col, t->cursor_row);
    SHOW_CURSOR;
  }

  fflush(stdout);
}

void cnc_terminal_update_screen_buffer(cnc_terminal *t)
{
  cnc_widget *w   = NULL;
  size_t sb_index = 0;
  size_t row = 0, col = 0;

  // for each widget
  for (size_t i = 0; i < t->widgets_count; i++)
  {
    w   = t->widgets[i];
    row = w->frame.origin.row;
    col = w->frame.origin.col;

    switch (w->type)
    {
    case WIDGET_INFO:
    {
      row++;
      cnc_buffer_replace(w->data, '\n', ' ');
      cnc_buffer_replace(w->data, '\r', USC);
      sb_index = _index_at_cr(t, col, row);

      cnc_buffer_replace_text(t->screen_buffer, sb_index - 10, 5, w->background,
                              0);

      cnc_buffer_replace_text(t->screen_buffer, sb_index - 5, 5, w->foreground,
                              0);

      cnc_buffer_replace_text(t->screen_buffer, sb_index, w->frame.width,
                              w->data->contents, w->index);

      // fill the line with spaces to overwrite old chars
      cnc_buffer_replace_char(t->screen_buffer, sb_index + w->data->length,
                              w->frame.width - w->data->length, ' ');
    }
    break;

    case WIDGET_PROMPT:
    {
      row++;
      sb_index = _index_at_cr(t, col, row);

      cnc_buffer_replace_text(t->screen_buffer, sb_index - 5, 5, w->foreground,
                              0);

      cnc_buffer_replace_text(
          t->screen_buffer, sb_index, 2,
          w->has_focus ? (t->mode == MODE_INS ? PROMPT_INS : PROMPT_CMD)
                       : PROMPT_NUL,
          0);

      cnc_buffer_replace_text(t->screen_buffer, sb_index + 2,
                              w->frame.width - 3, w->data->contents, w->index);

      // fill the line with spaces to overwrite old chars
      cnc_buffer_replace_char(t->screen_buffer, sb_index + w->data->length + 2,
                              w->frame.width - 2 - w->data->length, ' ');
    }
    break;

    case WIDGET_DISPLAY:
    {
      // split long stream into lines, without breaking words
      // an improtant option we have to consider is when the
      // user sends bf and fg information within the buffer.
      // in this case, those info will be ignored in the
      // char array, and added at the beginning of the line.
      // the only way to achieve this is to pick a color_info_byte
      // that will tell us that fg and bg info will follow.
      // this color_info_byte then will be ignored

      char *bg            = COLOR_NONE;
      char *fg            = COLOR_NONE;
      char *tmp_color     = COLOR_NONE;
      int color_operation = 0;

      size_t last_space_index = 0;
      size_t start_at_index   = 0;
      size_t length           = 0;
      size_t skip_rows        = 0;

      // flag to add newline if last character is not a newline
      bool add_new_line = (w->data->contents[w->data->length - 1] != '\n');

      // save number of rows in data_index
      w->data_index = 0;

      // add newline an the end of the buffer
      // needed for lines calculation
      if (add_new_line)
      {
        cnc_buffer_append(w->data, "\n");
      }

      // dry run to calculate the number of rows
      for (size_t i = 0; i < w->data->length; i++)
      {
        // do not account for color_info_byte
        if (w->data->contents[i] == COLOR_INFO_BYTE)
        {
          start_at_index += 2;
          i++;
          continue;
        }

        if (w->data->contents[i] == '\n' ||
            i - start_at_index >= w->frame.width)
        {
          w->data_index++;

          if (w->data->contents[i] == '\n')
          {
            length = i - start_at_index;
          }

          else if (start_at_index >= last_space_index)
          {
            length = i - start_at_index;
          }

          else
          {
            length = last_space_index - start_at_index;
          }

          row++;
          start_at_index += length;

          if (w->data->contents[i] == '\n')
          {
            start_at_index++;
          }
        }

        if (w->data->contents[i] == ' ')
        {
          last_space_index = i + 1;
        }
      }

      if (w->data_index > w->frame.height)
      {
        skip_rows = w->data_index - w->frame.height;
      }

      // Actual writing of data
      last_space_index = 0;
      start_at_index   = 0;
      length           = 0;

      // reset data_index
      w->data_index = 0;

      // reset row index
      row = w->frame.origin.row;

      for (size_t i = 0; i < w->data->length; i++)
      {
        // do not account for color_info_byte
        if (w->data->contents[i] == COLOR_INFO_BYTE)
        {
          color_operation =
              _color_code_to_color(w->data->contents[i + 1], &tmp_color);

          switch (color_operation)
          {
          case 0:
            bg = COLOR_NONE;
            fg = COLOR_NONE;
            break;
          case 1:
            bg = tmp_color;
            break;
          case 2:
            fg = tmp_color;
            break;

          default:
            break;
          }

          start_at_index += 2;
          i++;
          continue;
        }

        if (w->data->contents[i] == '\n' ||
            i - start_at_index >= w->frame.width)
        {
          w->data_index++;

          if (w->data->contents[i] == '\n')
          {
            length = i - start_at_index;
          }

          else if (start_at_index >= last_space_index)
          {
            length = i - start_at_index;
          }

          else
          {
            length = last_space_index - start_at_index;
          }

          if (row < w->frame.origin.row + w->frame.height &&
              w->data_index + w->index > skip_rows)
          {
            sb_index = _index_at_cr(t, col, row);

            // fill color data
            cnc_buffer_replace_text(t->screen_buffer, sb_index - 10, 5, bg, 0);
            cnc_buffer_replace_text(t->screen_buffer, sb_index - 5, 5, fg, 0);
            // write chars to line
            cnc_buffer_replace_text(t->screen_buffer, sb_index, length,
                                    w->data->contents, start_at_index);

            // fill the line with spaces to overwrite old chars
            cnc_buffer_replace_char(t->screen_buffer, sb_index + length,
                                    w->frame.width - length, ' ');

            row++;
          }

          start_at_index += length;

          if (w->data->contents[i] == '\n')
          {
            start_at_index++;
          }
        }

        if (w->data->contents[i] == ' ')
        {
          last_space_index = i + 1;
        }
      }

      // write any remaining chars to the last line.
      if (start_at_index < w->data->length)
      {
        sb_index = _index_at_cr(t, col, row);

        // fill color data
        cnc_buffer_replace_text(t->screen_buffer, sb_index - 10, 5, bg, 0);
        cnc_buffer_replace_text(t->screen_buffer, sb_index - 5, 5, fg, 0);

        // write chars to line
        cnc_buffer_replace_text(t->screen_buffer, sb_index,
                                w->data->length - start_at_index,
                                w->data->contents, start_at_index);

        // fill the line with spaces to overwrite old chars
        cnc_buffer_replace_char(
            t->screen_buffer, sb_index + w->data->length - start_at_index,
            w->frame.width - w->data->length + start_at_index, ' ');
      }

      // overwrite any old chars in the display
      for (size_t row_index = row; row_index <= w->frame.height; row_index++)
      {
        sb_index = _index_at_cr(t, col, row_index);
        cnc_buffer_replace_char(t->screen_buffer, sb_index, w->frame.width,
                                ' ');
      }

      // remove the newline from the end of the buffer
      // this was added for lines calculation
      if (add_new_line)
      {
        cnc_buffer_delete_char(w->data, w->data->length - 1);
      }
    }
    break;

    default:
      break;
    }
  }
}

void cnc_terminal_update_and_redraw(cnc_terminal *t)
{
  cnc_terminal_update_screen_buffer(t);
  cnc_terminal_redraw(t);
}

void cnc_terminal_set_row_fg(cnc_terminal *t, size_t row, const char *color)
{
  size_t sb_index = _index_at_cr(t, 1, row);

  cnc_buffer_replace_text(t->screen_buffer, sb_index - 5, 5, color, 0);
}

void cnc_terminal_set_row_bg(cnc_terminal *t, size_t row, const char *color)
{
  size_t sb_index = _index_at_cr(t, 1, row);

  cnc_buffer_replace_text(t->screen_buffer, sb_index - 10, 5, color, 0);
}

static int _cnc_terminal_getch(cnc_terminal *t)
{
  int bytes_read;
  char ch;
  int ch_sum = 0;

  ioctl(STDIN_FILENO, FIONREAD, &bytes_read);

  if (bytes_read <= 0)
  {
    return 0;
  }

  read(STDIN_FILENO, &ch, 1);

  if (bytes_read == 1)
  {
    return ch;
  }

  if (ch != KEY_ESCAPE)
  {
    return ch;
  }

  ch_sum = ch;

  for (int i = 0; i < bytes_read - 1; i++)
  {
    read(STDIN_FILENO, &ch, 1);
    ch_sum += ch;
  }

  return ch_sum;
}

static int _cnc_terminal_get_user_input(cnc_terminal *t)
{
  cnc_widget *fw = t->focused_widget;
  int result     = 0;

  while (result == 0)
  {
    result = _cnc_terminal_getch(t);

    if (t->mode == MODE_INS && result == CTRL_KEY('z'))
    {
      suspend_flag = 1;
    }

    _cnc_terminal_check_for_suspend(t);
    cnc_terminal_check_for_resize(t);

    usleep(10000);
  }

  // exit insert mode with ctrl-c
  if (t->mode == MODE_INS && result == CTRL_KEY('c'))
  {
    cnc_terminal_set_mode(t, MODE_CMD);
    return result;
  }

  // only when prompt has focus
  if (t->mode == MODE_INS && fw && fw->type == WIDGET_PROMPT)
  {
    // user presses ENTER key on a WIDGET_PROMPT
    if (result == KEY_ENTER || result == KEY_RETURN)
    {
      result = KEY_ENTER;
      return result;
    }

    // result is a valid character
    if (result >= ' ' && result <= '~')
    {
      _insert_char(fw, result);
      return result;
    }

    // result is backspace
    if (result == KEY_BACKSPACE)
    {
      _delete_char(fw);
      return result;
    }
  }

  switch (result)
  {
  case KEY_ARROW_UP:
  case 'k':
    _vim_mode_k(fw);
    return result;

  case KEY_ARROW_DN:
  case 'j':
    _vim_mode_j(fw);
    return result;

  case KEY_ARROW_RT:
  case 'l':
    _vim_mode_l(fw);
    return result;

  case KEY_ARROW_LT:
  case 'h':
    _vim_mode_h(fw);
    return result;

  case 'e':
    _vim_mode_e(fw);
    return result;

  case 'b':
    _vim_mode_b(fw);
    return result;

  case KEY_PAGE_UP:
    _page_up(fw);
    return result;

  case KEY_PAGE_DN:
    _page_dn(fw);
    return result;

  case KEY_ESCAPE:
    cnc_terminal_set_mode(t, MODE_CMD);
    return result;

  case KEY_INSERT:
  case 'i':
    cnc_terminal_set_mode(t, MODE_INS);
    return result;

  case KEY_TAB:
    cnc_terminal_focus_next(t);
    return result;

  case 'a':
    cnc_terminal_set_mode(t, MODE_INS);

    if (fw && fw->type == WIDGET_PROMPT && fw->data_index < fw->data->length)
    {
      fw->data_index++;
    }

    return result;

  case 'A':
    _vim_mode_$(fw);
    cnc_terminal_set_mode(t, MODE_INS);
    return result;

  case '0':
    _vim_mode_0(fw);
    return result;

  case '$':
    _vim_mode_$(fw);
    return result;

  case 'x':
    _vim_mode_x(fw);
    return result;

  default:
    return result;
  }
}

int cnc_terminal_get_user_input(cnc_terminal *t)
{
  int return_value = _cnc_terminal_get_user_input(t);
  cnc_terminal_update_and_redraw(t);

  return return_value;
}

void cnc_terminal_destroy(cnc_terminal *t)
{
  // restore terminal
  if (t->in_raw_mode)
  {
    _cnc_terminal_restore(t);
  }

  // destroy screen buffer
  cnc_buffer_destroy(t->screen_buffer);

  // destroy widgets
  for (size_t i = 0; i < t->widgets_count; i++)
  {
    if (t->widgets[i])
    {
      cnc_widget_destroy(t->widgets[i]);
    }
  }

  free(t->widgets);
  t->widgets = NULL;

  free(t);
  t = NULL;
}

// other useful functions
// Format Integer to C char array
const char *cnc_i_to_cca(int64_t num, bool thousand_separator, int8_t width)
{
  static char str[27];

  int8_t i      = 0;
  bool neg      = false;
  int8_t fwidth = (width < 0 || width > 26) ? 0 : width;

  if (num == INT64_MIN)
  {
    str[i++] = '-';
    num      = 9223372036854775807;
  }

  else if (num < 0)
  {
    neg = true;
    num = -num;
  }

  else if (num == 0)
  {
    str[i++] = '0';
  }

  else
  {
    int8_t start = i;

    while (num > 0)
    {
      str[i++] = (num % 10) + '0';
      num      = num / 10;
    }

    int8_t end = i - 1;

    while (start < end)
    {
      char temp  = str[start];
      str[start] = str[end];
      str[end]   = temp;
      start++;
      end--;
    }
  }

  if (thousand_separator)
  {
    int count = 0;

    for (int j = i - 1; j >= 0; j--)
    {
      count++;

      if (count % 3 == 0 && j > 0 && str[j] != '-')
      {
        for (int k = i; k >= j; k--)
        {
          str[k + 1] = str[k];
        }

        str[j] = ',';
        i++;
      }
    }
  }

  if (neg)
  {
    for (int j = i; j >= 0; j--)
    {
      str[j + 1] = str[j];
    }

    str[0] = '-';
    i++;
  }

  if (i < fwidth)
  {
    int padding = fwidth - i;

    for (int j = i; j >= 0; j--)
    {
      str[j + padding] = str[j];
    }

    for (int j = 0; j < padding; j++)
    {
      str[j] = ' ';
    }

    i = fwidth;
  }

  str[i] = '\0';

  return str;
}
