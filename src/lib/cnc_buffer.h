#ifndef CNC_BUFFER_H
#define CNC_BUFFER_H

// using cb as shorthand for cnc_buffer

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cnc_term_token.h"

// delete 25% when full
#define CB_SCROLL_FACTOR 4

// define buffer initial capacity
#define CB_INIT_CAP 256

typedef struct
{
  size_t size;
  size_t capacity;
  size_t max_capacity;

  cnc_term_token *data;

} cnc_buffer;

// functions
bool   cb_append_buf(cnc_buffer *dst, const cnc_buffer *src);
bool   cb_append_txt(cnc_buffer *cb, const char *text);
void   cb_clear(cnc_buffer *cb);
size_t cb_data_length(cnc_buffer *cb, size_t start_index, size_t count);
size_t cb_data_width(cnc_buffer *cb, size_t start_index, size_t count);
void   cb_destroy(cnc_buffer *cb);
bool   cb_equal_c_str(cnc_buffer *cb, char *str);

const cnc_term_token *cb_get(const cnc_buffer *cb, size_t index);

bool cb_init(cnc_buffer *cb, size_t max_capacity);
bool cb_insert(cnc_buffer *cb, const cnc_term_token token, size_t index);
bool cb_locate_buffer(cnc_buffer *cb, cnc_buffer *search, size_t *location);
bool cb_locate_c_str(cnc_buffer *cb, const char *str, size_t *location);
bool cb_overwrite(cnc_buffer *dst, size_t dst_start, size_t length,
                  cnc_buffer *src, size_t src_start);
bool cb_push(cnc_buffer *cb, const cnc_term_token token);
bool cb_remove(cnc_buffer *cb, size_t index);
bool cb_replace(cnc_buffer *cb, const cnc_term_token *match,
                const cnc_term_token *replacement);
bool cb_resize(cnc_buffer *cb, size_t new_capacity);
bool cb_set(cnc_buffer *cb, const cnc_term_token token, size_t index);
bool cb_set_text(cnc_buffer *cb, const char *text);

#endif
