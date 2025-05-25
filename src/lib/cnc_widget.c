#include "cnc_widget.h"

void cw_destroy(cnc_widget *cw)
{
  if (cw == NULL)
  {
    return;
  }

  cb_destroy(&cw->buffer);

  free(cw);
  cw = NULL;
}

cnc_widget *cw_init(cw_type type)
{
  cnc_widget *cw = malloc(sizeof(*cw));

  if (cw == NULL)
  {
    return NULL;
  }

  size_t buffer_size   = 0;
  cw->frame.origin.col = 1;
  cw->frame.origin.row = 1;
  cw->frame.height     = 2;
  cw->frame.width      = 1;

  cw->type = type;

  cw->index      = 0;
  cw->data_index = 0;

  cw->bg = ctt_parse_value(KS_NON___);
  cw->fg = ctt_parse_value(KS_NON___);

  cw->bg_main = ctt_parse_value(KS_NON___);
  cw->fg_main = ctt_parse_value(KS_NON___);

  cw->bg_alt = ctt_parse_value(KS_NON___);
  cw->fg_alt = ctt_parse_value(KS_NON___);

  cw->has_focus = false;

  switch (type)
  {
    case WIDGET_INFO:
      buffer_size   = INFO_BUFFER_SIZE;
      cw->bg_main   = ctt_parse_value(KS_GRE_BG);
      cw->fg_main   = ctt_parse_value(KS_BLA_FG);
      cw->bg_alt    = ctt_parse_value(KS_YEL_BG);
      cw->fg_alt    = ctt_parse_value(KS_BLA_FG);
      cw->can_focus = false;
      break;

    case WIDGET_PROMPT:
      buffer_size = PROMPT_BUFFER_SIZE;
      // cw->background = ctt_parse_value(KS_BLA_BG);
      cw->fg_main   = ctt_parse_value(KS_CYA_FG);
      cw->fg_alt    = ctt_parse_value(KS_RED_FG);
      cw->can_focus = true;
      break;

    case WIDGET_DISPLAY:
      buffer_size   = DISPLAY_BUFFER_SIZE;
      cw->can_focus = true;
      break;
  }

  cb_init(&cw->buffer, buffer_size);

  return cw;
}

void cw_reset(cnc_widget *cw)
{
  if (cw == NULL)
  {
    return;
  }

  cb_clear(&cw->buffer);
  cw->data_index = 0;
  cw->index      = 0;
}
