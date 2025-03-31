#include "ast.h"
#include "common.h"
#include "lex.h"

void run_tests(void) {
    common_test();
    lex_test();
    // print_test();
    parse_test();
}

int main(void) {
    run_tests();
    return 0;
}
