#include "cnc_terminal.h"

// Signals flags
volatile sig_atomic_t suspend_flag = 0;
volatile sig_atomic_t resize_flag  = 0;

static void __handle__sigtstp(int sig)
{
  suspend_flag = 1;
}

static void __handle__resize(int sig)
{
  resize_flag = 1;
}

// private functions declarations
static void _ct_check_for_suspend(cnc_terminal *ct);
static int  _ct_color_code_to_color(int color_code, cnc_term_token *color);
static void _ct_delete_char(cnc_widget *cw);
// static int  _ct_get_user_input(cnc_terminal *ct);

static cnc_term_token _ct_getch(cnc_terminal *ct);

static void _ct_insert_char(cnc_widget *cw, char c);
static void _ct_insert_token(cnc_widget *cw, cnc_term_token ctt_c);
static void _ct_page_dn(cnc_widget *cw);
static void _ct_page_up(cnc_widget *cw);
static void _ct_redraw(cnc_terminal *ct);
static void _ct_render_append_token(char **dst_ptr, cnc_term_token token);
static void _ct_render_border_row(char **buf_ptr, size_t row_width);
static void _ct_render_color_set(char **buf_ptr, cnc_term_token bg,
                                 cnc_term_token fg);
static void _ct_render_color_reset(char **buf_ptr);
static void _ct_render_data(char **buf_ptr, cnc_buffer *src, size_t start_index,
                            size_t upper_bound, size_t row_width);
static void _ct_render_empty_row(char **buf_ptr, size_t row_width);
static void _ct_render_enter(char **buf_ptr);
// static void _ct_render_save_row_index(size_t *u_bounds, size_t *rows,
//                                       size_t *row, size_t height,
//                                       size_t *counter,
//                                       size_t *first_start_index);
static void _ct_restore(cnc_terminal *ct);
static bool _ct_set_raw_mode(cnc_terminal *ct);

// Vim-Like functions
// vm -> vim_mode
static void _ct_vm_0(cnc_widget *cw);
static void _ct_vm_$(cnc_widget *cw);
static void _ct_vm_b(cnc_widget *cw);
static void _ct_vm_e(cnc_widget *cw);
static void _ct_vm_h(cnc_widget *cw);
static void _ct_vm_j(cnc_widget *cw);
static void _ct_vm_k(cnc_widget *cw);
static void _ct_vm_l(cnc_widget *cw);
static void _ct_vm_x(cnc_widget *cw);

// private function definitions
static void _ct_check_for_suspend(cnc_terminal *ct)
{
  if (ct == NULL)
  {
    return;
  }

  if (suspend_flag)
  {
    suspend_flag = 0;

    _ct_restore(ct);

    // Set default handler for SIGTSTP
    signal(SIGTSTP, SIG_DFL);

    // Send SIGTSTP to itself to suspend the process
    raise(SIGTSTP);
  }

  else
  {
    if (ct->in_raw_mode == false)
    {
      _ct_set_raw_mode(ct);
      ct_update(ct);
    }
  }
}

static int _ct_color_code_to_color(int color_code, cnc_term_token *color)
{
  if (color == NULL)
  {
    return 0;
  }

  switch (color_code)
  {
    case KS_BLA_FG:
      *color = ctt_parse_value(KS_BLA_FG);
      return 2;
    case KS_RED_FG:
      *color = ctt_parse_value(KS_RED_FG);
      return 2;
    case KS_GRE_FG:
      *color = ctt_parse_value(KS_GRE_FG);
      return 2;
    case KS_YEL_FG:
      *color = ctt_parse_value(KS_YEL_FG);
      return 2;
    case KS_BLU_FG:
      *color = ctt_parse_value(KS_BLU_FG);
      return 2;
    case KS_MAG_FG:
      *color = ctt_parse_value(KS_MAG_FG);
      return 2;
    case KS_CYA_FG:
      *color = ctt_parse_value(KS_CYA_FG);
      return 2;
    case KS_WHI_FG:
      *color = ctt_parse_value(KS_WHI_FG);
      return 2;
    case KS_BLA_BG:
      *color = ctt_parse_value(KS_BLA_BG);
      return 1;
    case KS_RED_BG:
      *color = ctt_parse_value(KS_RED_BG);
      return 1;
    case KS_GRE_BG:
      *color = ctt_parse_value(KS_GRE_BG);
      return 1;
    case KS_YEL_BG:
      *color = ctt_parse_value(KS_YEL_BG);
      return 1;
    case KS_BLU_BG:
      *color = ctt_parse_value(KS_BLU_BG);
      return 1;
    case KS_MAG_BG:
      *color = ctt_parse_value(KS_MAG_BG);
      return 1;
    case KS_CYA_BG:
      *color = ctt_parse_value(KS_CYA_BG);
      return 1;
    case KS_WHI_BG:
      *color = ctt_parse_value(KS_WHI_BG);
      return 1;

    default:
      return 0;
  }
}

static cnc_term_token _ct_getch(cnc_terminal *ct)
{
  if (ct == NULL)
  {
    return ctt_parse_value(0);
  }

  int bytes_read = 0;

  ioctl(STDIN_FILENO, FIONREAD, &bytes_read);

  if (bytes_read <= 0)
  {
    return ctt_parse_value(0);
  }

  uint8_t ch[CTT_MAX_TOKEN_SIZE] = {0};

  read(STDIN_FILENO, &ch[0], 1);

  if (bytes_read == 1)
  {
    return ctt_parse_value(ch[0]);
  }

  // UTF8 input
  if (ch[0] != C_ESC)
  {
    cnc_term_token ctt_utf8 = {0};

    int utf8_len =                     // get expected length
      ((ch[0] & 0x80) == 0x00   ? 1    // 0xxxxxxx
       : (ch[0] & 0xE0) == 0xC0 ? 2    // 110xxxxx
       : (ch[0] & 0xF0) == 0xE0 ? 3    // 1110xxxx
       : (ch[0] & 0xF8) == 0xF0 ? 4    // 11110xxx
                                : -1); // Invalid

    if (utf8_len == -1)
    {
      // Invalid UTF-8 start byte
      return ctt_parse_value(ch[0]);
    }

    // Already read 1 byte; read the rest
    for (int i = 1; i < utf8_len && i < bytes_read; ++i)
    {
      read(STDIN_FILENO, &ch[i], 1);

      if ((ch[i] & 0xC0) != 0x80)
      {
        // Invalid continuation byte
        return ctt_parse_value(ch[0]);
      }
    }

    // parse the bytes
    ctt_parse_bytes(ch, &ctt_utf8);

    return ctt_utf8;
  }

  // ch[0] is an escape => ANSI Escape Code
  uint32_t ch_sum = ch[0];

  for (int i = 1; i < bytes_read; i++)
  {
    read(STDIN_FILENO, &ch[i], 1);
    ch_sum += ch[i];
  }

  return ctt_parse_value(ch_sum);
}

static void _ct_insert_char(cnc_widget *cw, char c)
{
  cnc_term_token ctt_c = ctt_parse_value(c);
  _ct_insert_token(cw, ctt_c);
}

static void _ct_insert_token(cnc_widget *cw, cnc_term_token ctt_c)
{
  if (cw && cw->type == WIDGET_PROMPT)
  {
    if (cb_insert(&cw->buffer, ctt_c, cw->data_index))
    {
      cw->data_index++;

      if (cb_data_width(&cw->buffer, cw->index,
                        cw->data_index - cw->index + 1) +
            PROMPT_PAD + ctt_c.token.width >
          cw->frame.width)
      {
        ctt_width first_char_width = cw->buffer.data[cw->index].token.width;

        if (first_char_width == ctt_c.token.width)
        {
          cw->index++;
        }

        else
        {
          cw->index += ctt_c.token.width;
        }
      }
    }
  }
}

// TODO:
// page up and page down should run even if display is not selected...
static void _ct_page_dn(cnc_widget *cw)
{
  if (cw == NULL || cw->type != WIDGET_DISPLAY)
  {
    return;
  }

  if (cw->index + 2 * (cw->frame.height - 2) < cw->data_index)
  {
    cw->index += (cw->frame.height - 2);

    return;
  }

  if (cw->index + 2 * cw->frame.height > cw->data_index)
  {
    cw->index = cw->data_index - cw->frame.height;
  }
}

static void _ct_page_up(cnc_widget *cw)
{
  if (cw == NULL || cw->type != WIDGET_DISPLAY)
  {
    return;
  }

  if (cw->index > cw->frame.height)
  {
    cw->index -= (cw->frame.height - 2);
  }

  else
  {
    cw->index = 0;
  }
}

static void _ct_redraw(cnc_terminal *ct)
{
  if (ct == NULL)
  {
    return;
  }

  HIDE_CURSOR;
  HOME_POSITION;

  // check if terminal dimensions are within limits
  if (ct->scr_cols < ct->min_width || ct->scr_rows < ct->min_height)
  {
    printf(".. Please resize your terminal\n\r");
    printf(".. Terminal Size: R=%4zu, C=%4zu\n\r", ct->scr_rows, ct->scr_cols);
    printf("..      Min Size: R=%4zu, C=%4zu\n\r", ct->min_height,
           ct->min_width);

    fflush(stdout);
    return;
  }

  // write ct->screenbuffer to terminal
  write(STDOUT_FILENO, ct->screenbuffer, strlen(ct->screenbuffer));

  cnc_widget *cw = ct_focused_widget(ct);

  if (cw && cw->type == WIDGET_PROMPT)
  {
    cc_set_position(
      &ct->cursor, cw->frame.origin.row + 1,
      1 + PROMPT_PAD +
        cb_data_width(&cw->buffer, cw->index, cw->data_index - cw->index));

    POSCURSOR(ct->cursor.col, ct->cursor.row);
    SHOW_CURSOR;
  }

  fflush(stdout);
}

static void _ct_render_append_token(char **dst_ptr, cnc_term_token token)
{
  memcpy(*dst_ptr, token.seq, token.token.length);
  *dst_ptr += token.token.length;
}

static void _ct_render_border_row(char **buf_ptr, size_t row_width)
{
  cnc_term_token line_token = ctt_parse_value(KS_LIN___);

  for (size_t i = 0; i < row_width; ++i)
  {
    _ct_render_append_token(buf_ptr, line_token);
  }
}

static void _ct_render_color_set(char **buf_ptr, cnc_term_token bg,
                                 cnc_term_token fg)
{
  _ct_render_append_token(buf_ptr, bg);
  _ct_render_append_token(buf_ptr, fg);
}

static void _ct_render_color_reset(char **buf_ptr)
{
  cnc_term_token reset_sequence = ctt_parse_value(KS_RST___);
  _ct_render_append_token(buf_ptr, reset_sequence);
}

static void _ct_render_data(char **buf_ptr, cnc_buffer *src, size_t start_index,
                            size_t upper_bound, size_t row_width)
{
  /*
   * start_index -> first index of data
   * upper_bound -> last index of data (included)
   * row_width   -> total number of column to render
   */

  size_t width = 0;

  for (size_t i = start_index; i <= upper_bound; ++i)
  {
    cnc_term_token *t = &src->data[i];
    memcpy(*buf_ptr, t->seq, t->token.length);
    *buf_ptr += t->token.length;
    width += t->token.width;
  }

  memset(*buf_ptr, ' ', row_width - width);
  *buf_ptr += (row_width - width);
}

static void _ct_render_empty_row(char **buf_ptr, size_t row_width)
{
  memset(*buf_ptr, C_SPC, row_width);
  *buf_ptr += row_width;
}

static void _ct_render_enter(char **buf_ptr)
{
  **buf_ptr = '\r';
  (*buf_ptr)++;
  **buf_ptr = '\n';
  (*buf_ptr)++;
}

static void _ct_restore(cnc_terminal *ct)
{
  if (ct == NULL)
  {
    return;
  }

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &ct->orig_term);
  ct->in_raw_mode = false;

  // Restore cursor
  CURSOR_INS;
  SHOW_CURSOR;

  // Restore color
  write(STDOUT_FILENO, STR_RESET_STYLES, 5);

  // Clear screen
  CLRSCR;
  HOME_POSITION;
  fflush(stdout);
}

static bool _ct_set_raw_mode(cnc_terminal *ct)
{
  if (ct == NULL)
  {
    return false;
  }

  CLRSCR;
  HOME_POSITION;

  // read the current terminal attributes and store them
  if (tcgetattr(STDIN_FILENO, &ct->orig_term) == -1)
  {
    return false;
  }

  // put the terminal in raw mode
  struct termios raw_term = ct->orig_term;

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

  ct->in_raw_mode = true;

  return true;
}

static void _ct_vm_0(cnc_widget *cw)
{
  if (cw && cw->type == WIDGET_PROMPT)
  {
    cw->data_index = 0;
    cw->index      = 0;
  }

  if (cw && cw->type == WIDGET_DISPLAY)
  {
    if (cw->data_index > cw->frame.height)
    {
      cw->index = cw->data_index - cw->frame.height;
    }
  }
}

static void _ct_vm_$(cnc_widget *cw)
{
  if (cw && cw->type == WIDGET_PROMPT)
  {
    cnc_term_token ctt_c = cw->buffer.data[cw->buffer.size - 1];

    cw->data_index = cw->buffer.size;

    while (
      cb_data_width(&cw->buffer, cw->index, cw->data_index - cw->index + 1) +
        PROMPT_PAD + ctt_c.token.width >
      cw->frame.width)
    {

      cw->index++;
    }
  }

  if (cw && cw->type == WIDGET_DISPLAY)
  {
    cw->index = 0;
  }
}

static void _ct_vm_b(cnc_widget *cw)
{
  if (cw == NULL)
  {
    return;
  }

  bool move_backward      = cw->data_index > 0;
  bool curr_char_is_space = false;

  if (move_backward)
  {
    _ct_vm_h(cw);
    move_backward      = cw->data_index > 0;
    curr_char_is_space = cw->buffer.data[cw->data_index].token.value == C_SPC;
  }

  while (move_backward && curr_char_is_space)
  {
    _ct_vm_h(cw);
    move_backward      = cw->data_index > 0;
    curr_char_is_space = cw->buffer.data[cw->data_index].token.value == C_SPC;
  }

  while (move_backward && curr_char_is_space == false)
  {
    _ct_vm_h(cw);
    move_backward      = cw->data_index > 0;
    curr_char_is_space = cw->buffer.data[cw->data_index].token.value == C_SPC;
  }

  if (cw->data_index > 0)
  {
    _ct_vm_l(cw);
  }
}

static void _ct_vm_e(cnc_widget *cw)
{
  if (cw == NULL)
  {
    return;
  }

  bool move_forward       = cw->data_index < cw->buffer.size;
  bool curr_char_is_space = false;

  if (move_forward)
  {
    _ct_vm_l(cw);
    move_forward       = cw->data_index < cw->buffer.size;
    curr_char_is_space = cw->buffer.data[cw->data_index].token.value == C_SPC;
  }

  while (move_forward && curr_char_is_space)
  {
    _ct_vm_l(cw);
    move_forward       = cw->data_index < cw->buffer.size;
    curr_char_is_space = cw->buffer.data[cw->data_index].token.value == C_SPC;
  }

  while (move_forward && curr_char_is_space == false)
  {
    _ct_vm_l(cw);
    move_forward       = cw->data_index < cw->buffer.size;
    curr_char_is_space = cw->buffer.data[cw->data_index].token.value == C_SPC;
  }

  _ct_vm_h(cw);
}

// TODO:
static void _ct_vm_h(cnc_widget *cw)
{
  if (cw && cw->type == WIDGET_PROMPT && cw->data_index > 0)
  {
    cw->data_index--;

    size_t pad = PROMPT_PAD + 1 + 3;

    if (cw->index > 0 && cw->data_index - cw->index < cw->frame.width - pad)
    {
      cw->index--;
    }
  }
}

static void _ct_vm_j(cnc_widget *cw)
{
  if (cw && cw->type == WIDGET_DISPLAY &&
      cw->index + cw->frame.height < cw->data_index)
  {
    cw->index++;
  }
}

static void _ct_vm_k(cnc_widget *cw)
{
  if (cw && cw->type == WIDGET_DISPLAY && cw->index > 0)
  {
    cw->index--;
  }
}

static void _ct_vm_l(cnc_widget *cw)
{
  if (cw && cw->type == WIDGET_PROMPT && cw->data_index < cw->buffer.size)
  {
    cw->data_index++;

    if (cw->data_index - cw->index > cw->frame.width - PROMPT_PAD - 1)
    {
      cw->index++;
    }
  }
}

static void _ct_vm_x(cnc_widget *cw)
{
  if (cw && cw->type == WIDGET_PROMPT && cw->data_index < cw->buffer.size)
  {
    cb_remove(&cw->buffer, cw->data_index);
  }
}

// main functions
cnc_widget *ct_add_widget(cnc_terminal *ct, cw_type type)
{
  if (ct == NULL)
  {
    return NULL;
  }

  cnc_widget *cw = cw_init(type);

  if (cw == NULL)
  {
    return NULL;
  }

  size_t height = 0;

  for (size_t i = 0; i < ct->widgets_count; i++)
  {
    height += ct->widgets[i]->frame.height;
  }

  if (cw->frame.height + height > ct->scr_rows)
  {
    cw_destroy(cw);

    return NULL;
  }

  // resize the widgets array
  if (ct->widgets_count > 0)
  {
    cnc_widget **new_ct_widgets =
      realloc(ct->widgets, (ct->widgets_count + 1) * sizeof(*ct->widgets));

    if (new_ct_widgets == NULL)
    {
      cw_destroy(cw);

      return NULL;
    }

    ct->widgets = new_ct_widgets;
  }

  ct->widgets[ct->widgets_count++] = cw;

  ct_focus_widget(ct, cw);

  return cw;
}

void ct_check_for_resize(cnc_terminal *ct)
{
  if (ct == NULL)
  {
    return;
  }

  if (resize_flag)
  {
    resize_flag = 0;

    if (ct_get_size(ct) == false)
    {
      return;
    }

    CLRSCR;
    HOME_POSITION;

    size_t new_buffer_size = 1 + (ct->scr_cols + 16) * ct->scr_rows * 4;
    char  *new_buffer      = realloc(ct->screenbuffer, new_buffer_size);

    if (new_buffer == NULL)
    {
      ct_destroy(ct);
    }

    ct->screenbuffer      = new_buffer;
    ct->screenbuffer_size = new_buffer_size;

    memset(ct->screenbuffer, 0, ct->screenbuffer_size);

    cc_setup(&ct->cursor, ct->scr_rows, ct->scr_cols);

    ct_screenbuffer_reset(ct);
    ct_setup_widgets(ct);
    ct_update(ct);
  }
}

void ct_destroy(cnc_terminal *ct)
{
  if (ct == NULL)
  {
    return;
  }

  // restore terminal
  if (ct->in_raw_mode)
  {
    _ct_restore(ct);
  }

  // destroy rows info
  if (ct->rows_info)
  {
    free(ct->rows_info);
    ct->rows_info = NULL;
  }

  // destroy screenbuffer
  if (ct->screenbuffer)
  {
    free(ct->screenbuffer);
    ct->screenbuffer = NULL;
  }

  // destroy widgets
  for (size_t i = 0; i < ct->widgets_count; i++)
  {
    if (ct->widgets[i])
    {
      cw_destroy(ct->widgets[i]);
    }
  }

  free(ct->widgets);
  ct->widgets = NULL;

  free(ct);
  ct = NULL;
}

void ct_focus_next(cnc_terminal *ct)
{
  if (ct == NULL)
  {
    return;
  }

  if (ct->can_change_focus == false)
  {
    return;
  }

  int focused_index = -1;

  for (size_t i = 0; i < ct->widgets_count; i++)
  {
    if (ct->widgets[i]->has_focus)
    {
      focused_index = i;
      break;
    }
  }

  for (size_t i = 1; i <= ct->widgets_count; i++)
  {
    size_t index = (focused_index + i) % ct->widgets_count;

    if (ct->widgets[index]->can_focus)
    {
      ct_focus_widget(ct, ct->widgets[index]);
      break;
    }
  }
}

void ct_focus_widget(cnc_terminal *ct, cnc_widget *cw)
{
  if (ct == NULL || cw == NULL)
  {
    return;
  }

  if (cw->can_focus == false || cw->has_focus)
  {
    return;
  }

  for (size_t i = 0; i < ct->widgets_count; i++)
  {
    if (ct->widgets[i] != cw && ct->widgets[i]->has_focus)
    {
      ct->widgets[i]->has_focus = false;
      break;
    }
  }

  cw->has_focus      = true;
  ct->focused_widget = cw;
}

cnc_widget *ct_focused_widget(cnc_terminal *ct)
{
  if (ct)
  {
    for (size_t i = 0; i < ct->widgets_count; i++)
    {
      if (ct->widgets[i]->has_focus)
      {
        return ct->widgets[i];
      }
    }
  }

  return NULL;
}

bool ct_get_size(cnc_terminal *ct)
{
  if (ct == NULL)
  {
    return false;
  }

  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0 ||
      ws.ws_row == 0)
  {
    return false;
  }

  ct->scr_rows = ws.ws_row;
  ct->scr_cols = ws.ws_col;

  return true;
}

static void _ct_delete_char(cnc_widget *cw)
{
  if (cw && cw->type == WIDGET_PROMPT)
  {
    if (cw->data_index > 0)
    {
      if (cb_remove(&cw->buffer, cw->data_index - 1))
      {
        cw->data_index--;

        if (cw->index > 0)
        {
          cw->index--;
        }
      }
    }
  }
}

int ct_get_user_input(cnc_terminal *ct)
{
  if (ct == NULL)
  {
    return 0;
  }

  cnc_widget    *fw     = ct->focused_widget;
  int            result = 0;
  cnc_term_token ctt_result;

  while (result == 0)
  {
    ctt_result = _ct_getch(ct);
    result     = ctt_result.token.value;

    if (ct->mode == MODE_INS && result == CTRL_KEY('z'))
    {
      suspend_flag = 1;
    }

    _ct_check_for_suspend(ct);
    ct_check_for_resize(ct);

    usleep(10000);
  }

  // exit insert mode with ctrl-c
  if (ct->mode == MODE_INS && result == CTRL_KEY('c'))
  {
    ct_set_mode(ct, MODE_CMD);

    return result;
  }

  // only when prompt has focus
  if (ct->mode == MODE_INS && fw && fw->type == WIDGET_PROMPT)
  {
    // user presses ENTER key on a WIDGET_PROMPT
    if (result == C_ENT || result == C_RET)
    {
      result = C_ENT;

      return result;
    }

    // result is a valid character
    if (result >= C_SPC && result <= C_TLD)
    {
      _ct_insert_char(fw, result);

      return result;
    }

    // ctt_result is UTF8
    if (ctt_result.token.type == CTT_UTF8)
    {
      _ct_insert_token(fw, ctt_result);

      return ctt_result.token.value;
    }
  }

  switch (result)
  {
    case C_BCK:
      if (ct->mode == MODE_INS)
      {
        _ct_delete_char(fw);
      }
      return result;

    case KS_ARR_UP:
    case 'k':
      _ct_vm_k(fw);
      return result;

    case KS_ARR_DN:
    case 'j':
      _ct_vm_j(fw);
      return result;

    case KS_ARR_RT:
    case 'l':
      _ct_vm_l(fw);
      return result;

    case KS_ARR_LT:
    case 'h':
      _ct_vm_h(fw);
      return result;

    case 'e':
      _ct_vm_e(fw);
      return result;

    case 'b':
      _ct_vm_b(fw);
      return result;

    case KS_PAG_UP:
      _ct_page_up(fw);
      return result;

    case KS_PAG_DN:
      _ct_page_dn(fw);
      return result;

    case C_ESC:
      ct_set_mode(ct, MODE_CMD);
      return result;

    case KS_INS___:
    case 'i':
      ct_set_mode(ct, MODE_INS);
      return result;

    case C_TAB:
      ct_focus_next(ct);
      return result;

    case 'a':
      ct_set_mode(ct, MODE_INS);

      if (fw && fw->type == WIDGET_PROMPT && fw->data_index < fw->buffer.size)
      {
        fw->data_index++;
      }

      return result;

    case 'A':
      _ct_vm_$(fw);
      ct_set_mode(ct, MODE_INS);
      return result;

    case '0':
      _ct_vm_0(fw);
      return result;

    case '$':
      _ct_vm_$(fw);
      return result;

    case 'x':
      _ct_vm_x(fw);
      return result;

    default:
      return result;
  }
}

cnc_terminal *ct_init(size_t min_height, size_t min_width)
{
  cnc_terminal *ct = malloc(sizeof(*ct));

  if (ct == NULL)
  {
    return NULL;
  }

  // Setup the SIGTSTP signal handler
  ct->sa_sigtstp.sa_handler = __handle__sigtstp;
  ct->sa_sigtstp.sa_flags   = SA_RESTART;
  sigemptyset(&ct->sa_sigtstp.sa_mask);

  if (sigaction(SIGTSTP, &ct->sa_sigtstp, NULL) == -1)
  {
    ct_destroy(ct);

    return NULL;
  }

  // Setup resize signal handling
  ct->sa_resize.sa_handler = __handle__resize;
  ct->sa_resize.sa_flags   = SA_RESTART;
  sigemptyset(&ct->sa_resize.sa_mask);

  if (sigaction(SIGWINCH, &ct->sa_resize, NULL) == -1)
  {
    ct_destroy(ct);

    return NULL;
  }

  // initialize terminal settings
  ct->min_width  = min_width;
  ct->min_height = min_height;

  ct->scr_rows         = 0;
  ct->scr_cols         = 0;
  ct->in_raw_mode      = false;
  ct->can_change_mode  = true;
  ct->can_change_focus = true;
  ct->widgets_count    = 0;

  // Allocate memory for widgets
  ct->widgets = malloc(sizeof(*ct->widgets));

  if (ct->widgets == NULL)
  {
    ct_destroy(ct);

    return NULL;
  }

  // no widget has focus at the beginning
  ct->focused_widget = NULL;

  if (_ct_set_raw_mode(ct) == false)
  {
    ct_destroy(ct);

    return NULL;
  }

  // get the terminal size
  if (ct_get_size(ct) == false)
  {
    ct_destroy(ct);

    return NULL;
  }

  // setup cursor data
  cc_setup(&ct->cursor, ct->scr_rows, ct->scr_cols);

  // ct->screenbuffer memory allocation:
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

  ct->screenbuffer_size = 1 + (ct->scr_cols + 16) * ct->scr_rows * 4;
  ct->screenbuffer      = malloc(ct->screenbuffer_size);

  if (ct->screenbuffer == NULL)
  {
    ct_destroy(ct);

    return NULL;
  }

  memset(ct->screenbuffer, 0, ct->screenbuffer_size);

  // allocate memory for display rows info
  // if all the elements of a display would be '\n', we would have a max of
  // DISPLAY_BUFFER_SIZE rows to save info for the lifetime of the program

  ct->rows_info = malloc(DISPLAY_BUFFER_SIZE * sizeof(*ct->rows_info));

  if (ct->rows_info == NULL)
  {
    ct_destroy(ct);

    return NULL;
  }

  memset(ct->rows_info, 0, DISPLAY_BUFFER_SIZE * sizeof(*ct->rows_info));

  // put terminal in command mode
  ct_set_mode(ct, MODE_CMD);

  // empty the buffer and redraw (clear the screen)
  ct_screenbuffer_reset(ct);
  _ct_redraw(ct);

  return ct;
}

void ct_screenbuffer_reset(cnc_terminal *ct)
{
  if (ct == NULL)
  {
    return;
  }

  // const char c = ' ';
  // size_t sb_i; // screen_buffer index

  /*
  for (size_t i = 0; i < t->scr_rows; i++)
  {
    // number of bytes in each row:
    //  - color bg      : 5
    //  - color fg      : 5
    //  - total cols    : t->scr_cols
    //  - color reset   : 4
    //  - '\n\r'        : 2
    sb_i = _index_at_cr(t, 1, i + 1);
    cnc_buffer_insert_text(t->screen_buffer, sb_i - 10, 5, COLOR_NONE);
    cnc_buffer_insert_text(t->screen_buffer, sb_i - 5, 5, COLOR_NONE);
    cnc_buffer_insert_char(t->screen_buffer, sb_i, t->scr_cols, c);
    cnc_buffer_insert_text(t->screen_buffer, sb_i + t->scr_cols, 4,
                           COLOR_DEFAULT);

    if (i != t->scr_rows - 1)
    {
      cnc_buffer_insert_char(t->screen_buffer, sb_i + 4 + t->scr_cols, 1, '\n');
      cnc_buffer_insert_char(t->screen_buffer, sb_i + 5 + t->scr_cols, 1, '\r');
    }
  }
  */
}

void ct_set_mode(cnc_terminal *ct, ct_mode mode)
{
  if (ct == NULL)
  {
    return;
  }

  if (ct->can_change_mode == false)
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

  ct->mode = mode;
}

bool ct_setup_widgets(cnc_terminal *ct)
{
  if (ct == NULL)
  {
    return false;
  }

  size_t tlw        = 0; // number of two_lines_widgets
  size_t mlw        = 0; // number of multi_line_widgets
  size_t global_row = 1;

  for (size_t i = 0; i < ct->widgets_count; i++)
  {
    switch (ct->widgets[i]->type)
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

  if (ct->scr_rows < (2 * tlw + 3 * (mlw)))
  {
    return false;
  }

  // Calculate locations and dimensions
  size_t set_display_widget     = 1;
  size_t display_widgets_height = 0;

  for (size_t i = 0; i < ct->widgets_count; i++)
  {
    // set correct width and resize widgets lines
    ct->widgets[i]->frame.origin.row = global_row;

    if (ct->widgets[i]->type == WIDGET_DISPLAY)
    {
      if (set_display_widget < mlw)
      {
        ct->widgets[i]->frame.height = (ct->scr_rows - 2 * tlw) / mlw;
        display_widgets_height += ct->widgets[i]->frame.height;
        set_display_widget++;
      }

      else
      {
        ct->widgets[i]->frame.height =
          ct->scr_rows - 2 * tlw - display_widgets_height;
      }
    }

    ct->widgets[i]->frame.width = ct->scr_cols;
    global_row += ct->widgets[i]->frame.height;
  }

  return true;
}

void ct_update(cnc_terminal *ct)
{
  if (ct == NULL)
  {
    return;
  }

  cnc_term_token token_enter  = ctt_parse_value(C_ENT);
  cnc_term_token token_return = ctt_parse_value(C_RET);
  cnc_term_token token_space  = ctt_parse_value(C_SPC);
  cnc_term_token token_blank  = ctt_parse_value(C_USC);

  // clear previous screenbuffer
  memset(ct->screenbuffer, 0, ct->screenbuffer_size);

  char *buf_ptr = ct->screenbuffer;

  for (size_t widget_index = 0; widget_index < ct->widgets_count;
       ++widget_index)
  {
    cnc_widget *cw = ct->widgets[widget_index];

    if (cw == NULL)
    {
      continue;
    }

    switch (cw->type)
    {
      case WIDGET_INFO:
      case WIDGET_PROMPT:
      {
        cb_replace(&cw->buffer, &token_enter, &token_space);
        cb_replace(&cw->buffer, &token_return, &token_blank);

        // Skip a line
        _ct_render_border_row(&buf_ptr, ct->scr_cols);
        _ct_render_enter(&buf_ptr);

        // BG and FG
        // user defined bg and fg overwrite system defaults
        if (cw->bg.token.value != KS_NON___ || cw->fg.token.value != KS_NON___)
        {
          _ct_render_color_set(&buf_ptr, cw->bg, cw->fg);
        }

        else
        {
          // setting up WIDGET_PROMPT Colors
          if (cw->type == WIDGET_PROMPT)
          {
            if (cw->has_focus)
            {
              _ct_render_color_set(&buf_ptr, cw->bg_main, cw->fg_main);
            }

            else
            {
              _ct_render_color_set(&buf_ptr, cw->bg_alt, cw->fg_alt);
            }
          }

          // setting up WIDGET_INFO Colors
          else
          {
            if (ct->focused_widget->type == WIDGET_DISPLAY)
            {
              _ct_render_color_set(&buf_ptr, cw->bg_alt, cw->fg_alt);
            }

            else
            {
              _ct_render_color_set(&buf_ptr, cw->bg_main, cw->fg_main);
            }
          }
        }

        size_t line_width = ct->scr_cols;
        size_t padding    = 0;

        if (cw->type == WIDGET_PROMPT)
        {
          // Prompt Symbol
          memcpy(buf_ptr, ct->mode == MODE_INS ? PROMPT_INS : PROMPT_CMD,
                 PROMPT_PAD);

          buf_ptr += PROMPT_PAD;
          line_width -= (PROMPT_PAD + 1);
          padding = PROMPT_PAD;
        }

        // Text
        size_t u_bound = cw->buffer.size;
        size_t width;

        if (cw->buffer.size == 0)
        {
          _ct_render_empty_row(&buf_ptr, ct->scr_cols - padding);
        }

        else
        {
          do
          {
            u_bound--;
            width =
              cb_data_width(&cw->buffer, cw->index, u_bound - cw->index + 1);

          } while (width > line_width);

          _ct_render_data(&buf_ptr, &cw->buffer, cw->index, u_bound,
                          ct->scr_cols - padding);
        }

        _ct_render_color_reset(&buf_ptr);

        if (widget_index < ct->widgets_count - 1)
        {
          _ct_render_enter(&buf_ptr);
        }
      }
      break;

      case WIDGET_DISPLAY:
      {
        // split long strings into lines, without breaking words
        // an improtant option we have to consider is when the
        // user sends bg and fg information within the buffer.
        // in this case, those info will be ignored in the
        // char array, and added at the beginning of the line.
        // one way to achieve this is to pick a color_info_byte
        // that will tell us that fg and bg info will follow.
        // this color_info_byte then will be ignored

        size_t row              = 0;
        size_t counter          = 0;
        size_t row_width        = 0;
        size_t last_space_index = 0;
        bool   add_new_line     = false;

        ct_row_info     row_info      = {0};
        cnc_term_token *counter_token = NULL;

        // clear old rows_info
        memset(ct->rows_info, 0, DISPLAY_BUFFER_SIZE * sizeof(*ct->rows_info));
        size_t row_index = 0;

        cw->data_index = 0;

        if (cw->buffer.size == 0)
        {
          for (row = 0; row < cw->frame.height; row++)
          {
            _ct_render_enter(&buf_ptr);
          }
          break;
        }

        // flag to add newline if last character is not a newline
        counter_token = &(cw->buffer.data[cw->buffer.size - 1]);
        add_new_line  = counter_token->token.value != C_ENT;

        if (add_new_line)
        {
          cb_push(&cw->buffer, ctt_parse_value(C_ENT));
        }

        // calculation phase
        while (counter < cw->buffer.size)
        {
          counter_token = &cw->buffer.data[counter];

          row_width = cb_data_width(&cw->buffer, row_info.first_index,
                                    counter - row_info.first_index + 1);

          if (counter_token->token.value == C_ENT)
          {
            cw->data_index++;

            if (counter > 0)
            {
              row_info.last_index = counter - 1;
            }

            else
            {
              row_info.last_index = 0;
            }

            row_info.is_valid          = 1;
            ct->rows_info[row_index++] = row_info;
            row_info.first_index = ct->rows_info[row_index - 1].last_index + 2;

            row_info.bg = CTT_PV(0);
            row_info.fg = CTT_PV(0);
            counter++;

            continue;
          }

          if (row_width > ct->scr_cols)
          {
            cw->data_index++;

            // Finalize the current row info
            if (last_space_index > 0)
            {
              row_info.last_index = last_space_index - 1;
            }

            else
            {
              row_info.last_index = 0;
            }

            row_info.is_valid          = 1;
            ct->rows_info[row_index++] = row_info;

            // Advance counter to the next non-space token
            counter = last_space_index + 1;

            // skip any addititonal spaces
            while (counter < cw->buffer.size &&
                   ctt_is_whitespace(cw->buffer.data[counter]))
            {
              counter++;
            }

            // prepare next row_info
            row_info.first_index = counter;
            row_info.bg          = CTT_PV(0);
            row_info.fg          = CTT_PV(0);

            continue;
          }

          if (ctt_is_whitespace(*counter_token))
          {
            // redefine last word
            last_space_index = counter++;

            continue;
          }

          // Colors decryption
          if (counter_token->token.value == C_COL)
          {
            cnc_term_token token_color_value;

            int color_operation = _ct_color_code_to_color(
              (counter_token + 1)->token.value, &token_color_value);

            if (color_operation == 1)
            {
              row_info.bg = token_color_value;
            }

            if (color_operation == 2)
            {
              row_info.fg = token_color_value;
            }

            counter += 2;
            continue;
          }

          counter++;
        }

        // rendering phase
        for (row = 0; row < cw->frame.height; row++)
        {
          size_t r = row + cw->index;

          if (ct->rows_info[r].is_valid == 0)
          {
            break;
          }

          if (ct->rows_info[r].bg.token.value > 0)
          {
            _ct_render_append_token(&buf_ptr, ct->rows_info[r].bg);
          }

          if (ct->rows_info[r].fg.token.value > 0)
          {
            _ct_render_append_token(&buf_ptr, ct->rows_info[r].fg);
          }

          _ct_render_data(&buf_ptr, &cw->buffer, ct->rows_info[r].first_index,
                          ct->rows_info[r].last_index, ct->scr_cols);
          _ct_render_color_reset(&buf_ptr);

          if (row < cw->frame.height - 1)
          {
            _ct_render_enter(&buf_ptr);
          }
        }

        while (row < cw->frame.height)
        {
          _ct_render_empty_row(&buf_ptr, ct->scr_cols);
          _ct_render_enter(&buf_ptr);

          row++;
        }

        // remove the newline from the end of the buffer
        // this was added for lines calculation
        if (add_new_line)
        {
          cb_remove(&cw->buffer, cw->buffer.size - 1);
        }
      }
      break;
    }
  }

  // add null termination
  *buf_ptr = '\0';

  // redraw the terminal
  _ct_redraw(ct);
}
