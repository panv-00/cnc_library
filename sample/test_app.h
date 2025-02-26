#ifndef TEST_APP_H
#define TEST_APP_H

#include "../cnc_library.h"

#define APP_VERSION     "0.1.1"
#define TERM_MIN_WIDTH  80
#define TERM_MIN_HEIGHT 22

void free_all_allocations(cnc_terminal *);
int main(void);

#endif
