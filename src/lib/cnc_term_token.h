#ifndef CNC_TERM_TOKEN_H
#define CNC_TERM_TOKEN_H

// using ctt as shorthand for cnc_term_token

#include "cnc_term_token_width.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
    Code point ↔ UTF-8 conversion
    -----------------------------
    |   *From* |     *To* || *Byte 1* | *Byte 2* | *Byte 3* | *Byte 4* |
    | -------- | -------- || -------- | -------- | -------- | -------- |
    |   U+0000 |   U+007F || 0yyyzzzz |          |          |          |
    |   U+0080 |   U+07FF || 110xxxyy | 10yyzzzz |          |          |
    |   U+0800 |   U+FFFF || 1110wwww | 10xxxxyy | 10yyzzzz |          |
    | U+010000 | U+10FFFF || 11110uvv | 10vvwwww | 10xxxxyy | 10yyzzzz |
*/

// max token size
#define CTT_MAX_TOKEN_SIZE 12

// simple <Ctrl>[key] manipulation
#define CTRL_KEY(k) ((k) & 0x1f)

#define W_0 0  // last byte shift value
#define W_1 6  // ...
#define W_2 12 // ...
#define W_3 18 // ...

// ASCII values Table
#define C_COL 0x05 // Color Info Byte
#define C_TAB 0x09 // Tab
#define C_ENT 0x0A // Enter
#define C_RET 0x0D // Return
#define C_ESC 0x1B // Escape
#define C_USC 0x1F // used as blank
#define C_SPC 0x20 // space
#define C_CSI 0x5B // '['
#define C_TLD 0x7E // Tilde
#define C_BCK 0x7F // Backspace

// Empty 5 bytes sequence
#define KS_NON___ 155 // Empty: C_USC C_USC C_USC C_USC C_USC

// straight line char
#define KS_LIN___ 0x2500

// ANSI Cursor Key Sequence values Table
#define KS_ARR_UP 183 // C_ESC C_CSI A
#define KS_ARR_DN 184 // C_ESC C_CSI B
#define KS_ARR_RT 185 // C_ESC C_CSI C
#define KS_ARR_LT 186 // C_ESC C_CSI D
#define KS_END___ 188 // C_ESC C_CSI F
#define KS_HOM___ 190 // C_ESC C_CSI H
#define KS_INS___ 294 // C_ESC C_CSI 2 C_TILDE
#define KS_DEL___ 295 // C_ESC C_CSI 3 C_TILDE
#define KS_PAG_UP 297 // C_ESC C_CSI 5 C_TILDE
#define KS_PAG_DN 298 // C_ESC C_CSI 6 C_TILDE

// ANSI Colors and Effects Key Sequence values Table
#define KS_RST___ 227 // Reset:       C_ESC C_CSI  0 C_m
#define KS_BLD___ 228 // Bold:        C_ESC C_CSI  1 C_m
#define KS_DIM___ 229 // Dim:         C_ESC C_CSI  2 C_m
#define KS_ITA___ 230 // Italic:      C_ESC C_CSI  3 C_m
#define KS_ULN___ 231 // Underline:   C_ESC C_CSI  4 C_m
#define KS_BLS___ 232 // Blink, Slow: C_ESC C_CSI  5 C_m
#define KS_BLR___ 233 // Blink, Fast: C_ESC C_CSI  6 C_m
#define KS_INV___ 234 // Invert:      C_ESC C_CSI  7 C_m
#define KS_HID___ 235 // Hidden:      C_ESC C_CSI  8 C_m
#define KS_STK___ 236 // Strikeout:   C_ESC C_CSI  9 C_m
#define KS_BLA_FG 257 // Black FG:    C_ESC C_CSI 30 C_m
#define KS_RED_FG 258 // Red FG:      C_ESC C_CSI 31 C_m
#define KS_GRE_FG 259 // Green FG:    C_ESC C_CSI 32 C_m
#define KS_YEL_FG 260 // Yellow FG:   C_ESC C_CSI 33 C_m
#define KS_BLU_FG 261 // Blue FG:     C_ESC C_CSI 34 C_m
#define KS_MAG_FG 262 // Magenta FG:  C_ESC C_CSI 35 C_m
#define KS_CYA_FG 263 // Cyan FG:     C_ESC C_CSI 36 C_m
#define KS_WHI_FG 264 // White FG:    C_ESC C_CSI 37 C_m
#define KS_BLA_BG 267 // Black BG:    C_ESC C_CSI 40 C_m
#define KS_RED_BG 268 // Red BG:      C_ESC C_CSI 41 C_m
#define KS_GRE_BG 269 // Green BG:    C_ESC C_CSI 42 C_m
#define KS_YEL_BG 270 // Yellow BG:   C_ESC C_CSI 43 C_m
#define KS_BLU_BG 271 // Blue BG:     C_ESC C_CSI 44 C_m
#define KS_MAG_BG 272 // Magenta BG:  C_ESC C_CSI 45 C_m
#define KS_CYA_BG 273 // Cyan BG:     C_ESC C_CSI 46 C_m
#define KS_WHI_BG 274 // White BG:    C_ESC C_CSI 47 C_m

typedef enum
{
  CTT_NONE     = 0,  // Invalid or uninitialized token
  CTT_CHAR     = 1,  // Single ASCII character (1 byte)
  CTT_UTF8     = 4,  // UTF-8 character (max 4 bytes)
  CTT_A_STYLE  = 5,  // ANSI style/color sequences (ESC[42m), max 5 bytes
  CTT_A_CURSOR = 12, // ANSI cursor sequences (ESC[9999;9999H), max 12 bytes

} ctt_type;

typedef enum
{
  W_NIL = 0, // 0 display byte
  W_ONE = 1, // 1 column width byte
  W_TWO = 2  // 2 columns width byte

} ctt_width;

typedef struct
{
  uint32_t  value  : 21; // token numerical value
  ctt_width width  : 3;
  ctt_type  type   : 4;
  uint8_t   length : 4; // 0 to 12, depending on type

} __attribute__((packed)) cnc_term_token_compact;

typedef struct
{
  cnc_term_token_compact token;

  uint8_t seq[CTT_MAX_TOKEN_SIZE]; // token bytes array

} cnc_term_token;

static const cnc_term_token ctt_ansi_tokens[] = {
  {{KS_ARR_UP, W_NIL, CTT_A_CURSOR, 3}, {C_ESC, C_CSI, 'A'}                },
  {{KS_ARR_DN, W_NIL, CTT_A_CURSOR, 3}, {C_ESC, C_CSI, 'B'}                },
  {{KS_ARR_RT, W_NIL, CTT_A_CURSOR, 3}, {C_ESC, C_CSI, 'C'}                },
  {{KS_ARR_LT, W_NIL, CTT_A_CURSOR, 3}, {C_ESC, C_CSI, 'D'}                },
  {{KS_END___, W_NIL, CTT_A_CURSOR, 3}, {C_ESC, C_CSI, 'F'}                },
  {{KS_HOM___, W_NIL, CTT_A_CURSOR, 3}, {C_ESC, C_CSI, 'H'}                },
  {{KS_INS___, W_NIL, CTT_A_CURSOR, 4}, {C_ESC, C_CSI, '2', C_TLD}         },
  {{KS_DEL___, W_NIL, CTT_A_CURSOR, 4}, {C_ESC, C_CSI, '3', C_TLD}         },
  {{KS_PAG_UP, W_NIL, CTT_A_CURSOR, 4}, {C_ESC, C_CSI, '5', C_TLD}         },
  {{KS_PAG_DN, W_NIL, CTT_A_CURSOR, 4}, {C_ESC, C_CSI, '6', C_TLD}         },

  {{KS_RST___, W_NIL, CTT_A_STYLE, 4},  {C_ESC, C_CSI, '0', 'm'}           },
  {{KS_BLD___, W_NIL, CTT_A_STYLE, 4},  {C_ESC, C_CSI, '1', 'm'}           },
  {{KS_DIM___, W_NIL, CTT_A_STYLE, 4},  {C_ESC, C_CSI, '2', 'm'}           },
  {{KS_ITA___, W_NIL, CTT_A_STYLE, 4},  {C_ESC, C_CSI, '3', 'm'}           },
  {{KS_ULN___, W_NIL, CTT_A_STYLE, 4},  {C_ESC, C_CSI, '4', 'm'}           },
  {{KS_BLS___, W_NIL, CTT_A_STYLE, 4},  {C_ESC, C_CSI, '5', 'm'}           },
  {{KS_BLR___, W_NIL, CTT_A_STYLE, 4},  {C_ESC, C_CSI, '6', 'm'}           },
  {{KS_INV___, W_NIL, CTT_A_STYLE, 4},  {C_ESC, C_CSI, '7', 'm'}           },
  {{KS_HID___, W_NIL, CTT_A_STYLE, 4},  {C_ESC, C_CSI, '8', 'm'}           },
  {{KS_STK___, W_NIL, CTT_A_STYLE, 4},  {C_ESC, C_CSI, '9', 'm'}           },

  {{KS_NON___, W_NIL, CTT_A_STYLE, 5},  {C_USC, C_USC, C_USC, C_USC, C_USC}},

  {{KS_BLA_FG, W_NIL, CTT_A_STYLE, 5},  {C_ESC, C_CSI, '3', '0', 'm'}      },
  {{KS_RED_FG, W_NIL, CTT_A_STYLE, 5},  {C_ESC, C_CSI, '3', '1', 'm'}      },
  {{KS_GRE_FG, W_NIL, CTT_A_STYLE, 5},  {C_ESC, C_CSI, '3', '2', 'm'}      },
  {{KS_YEL_FG, W_NIL, CTT_A_STYLE, 5},  {C_ESC, C_CSI, '3', '3', 'm'}      },
  {{KS_BLU_FG, W_NIL, CTT_A_STYLE, 5},  {C_ESC, C_CSI, '3', '4', 'm'}      },
  {{KS_MAG_FG, W_NIL, CTT_A_STYLE, 5},  {C_ESC, C_CSI, '3', '5', 'm'}      },
  {{KS_CYA_FG, W_NIL, CTT_A_STYLE, 5},  {C_ESC, C_CSI, '3', '6', 'm'}      },
  {{KS_WHI_FG, W_NIL, CTT_A_STYLE, 5},  {C_ESC, C_CSI, '3', '7', 'm'}      },

  {{KS_BLA_BG, W_NIL, CTT_A_STYLE, 5},  {C_ESC, C_CSI, '4', '0', 'm'}      },
  {{KS_RED_BG, W_NIL, CTT_A_STYLE, 5},  {C_ESC, C_CSI, '4', '1', 'm'}      },
  {{KS_GRE_BG, W_NIL, CTT_A_STYLE, 5},  {C_ESC, C_CSI, '4', '2', 'm'}      },
  {{KS_YEL_BG, W_NIL, CTT_A_STYLE, 5},  {C_ESC, C_CSI, '4', '3', 'm'}      },
  {{KS_BLU_BG, W_NIL, CTT_A_STYLE, 5},  {C_ESC, C_CSI, '4', '4', 'm'}      },
  {{KS_MAG_BG, W_NIL, CTT_A_STYLE, 5},  {C_ESC, C_CSI, '4', '5', 'm'}      },
  {{KS_CYA_BG, W_NIL, CTT_A_STYLE, 5},  {C_ESC, C_CSI, '4', '6', 'm'}      },
  {{KS_WHI_BG, W_NIL, CTT_A_STYLE, 5},  {C_ESC, C_CSI, '4', '7', 'm'}      },
};

#define CTT_PV(c) ctt_parse_value(c)

// main functions
bool ctt_equal(const cnc_term_token *tkn1, const cnc_term_token *tkn2);
bool ctt_is_whitespace(cnc_term_token tkn);
bool ctt_parse_bytes(uint8_t *bytes, cnc_term_token *ctt);
cnc_term_token ctt_parse_value(uint32_t value);

#endif
