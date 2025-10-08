#include <iostream>

#include <parser/parser.h>

int main() {
    core::Parser parser;

    std::cout << parser.Evaluate("1 + 10 * (5 + 5)") << '\n';
    
    return 0;
}