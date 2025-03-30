#include "common.h"
#include "lex.h"

void run_tests(void) {
    lex_test();
    common_test();
}

int main(void) {
    run_tests();
    return 0;
}
