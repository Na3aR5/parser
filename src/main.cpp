#include <iostream>
#include <cstring>

#include <parser/parser.h>

int main() {
    core::Parser parser;

    std::string command;

    while (true) {
        std::cout << ": ";
        std::getline(std::cin, command);
        if (strcmp(command.c_str(), "close") == 0) {
            break;
        }
        else if (strcmp(command.c_str(), "eval") == 0) {
            command.clear();
            std::getline(std::cin, command);
            auto result = parser.Evaluate(command.c_str());
            if (result.HasValue()) {
                std::cout << "Result: " << result.Get() << "\n\n";
            }
            else {
                std::cout << "Expression is invalid (code: " << (int)result.Error() << ")\n\n";
            }
        }
        else {
            std::cout << "Unknown command\n\n";
        }

        command.clear();
    }
    
    return 0;
}