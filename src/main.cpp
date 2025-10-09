#include <iostream>
#include <cstring>

#include <parser/parser.h>

int main() {
    core::Parser parser;

    std::string command;

    while (true) {
        std::cout << ": ";
        std::getline(std::cin, command);
        if (strcmp(command.c_str(), "-quit") == 0) {
            break;
        }
        else if (strcmp(command.c_str(), "-eval") == 0) {
            command.clear();
            std::getline(std::cin, command);
            std::cout << "Result: " << parser.Evaluate(command.c_str()) << '\n';
        }
        else {
            std::cout << "Unknown command\n";
        }

        command.clear();
    }
    
    return 0;
}