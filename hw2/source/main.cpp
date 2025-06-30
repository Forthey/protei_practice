#include <iostream>
#include <map>

#include "FileHandler.h"


void printHelp() {
    std::cout << "<filename> <mode>\n"
                 "Mode: r/w/x (read/write/append)\n"
                 "Example: example.txt w\n"
                 "exit() to exit\n";
}

int main() {
    std::map<std::string, FileHandler::FileMode> modes = {
        {"r", FileHandler::FileMode::Read},
        {"w", FileHandler::FileMode::Write},
        {"a", FileHandler::FileMode::Append}
    };

    while (true) {
        printHelp();
        std::cout << "> ";

        std::string filename, mode;
        std::cin >> filename;

        if (filename == "exit()") {
            exit(0);
        }

        std::cin >> mode;
        if (!modes.count(mode)) {
            std::cerr << "Invalid Mode" << std::endl;
            continue;
        }

        try {
            FileHandler handler(filename, modes[mode]);

            std::cout << "File opened: " << filename << std::endl;
            std::cin.ignore();

            std::optional<std::string> line("");
            switch (modes[mode]) {
            case FileHandler::FileMode::Write:
            case FileHandler::FileMode::Append:
                std::cout << "Enter the contents of the file line by line (double-pressing Enter stops typing)\n";
                while (std::cin.good()) {
                    std::cout << "> ";
                    std::getline(std::cin, line.value());

                    if (line.value().empty()) {
                        break;
                    }
                    handler.writeLine(line.value());
                }
                break;
            case FileHandler::FileMode::Read:
                while ((line = handler.readLine()).has_value()) {
                    std::cout << line.value() << std::endl;
                }
                break;
            }
        } catch (FileHandler::FileException const& e) {
            std::cerr << e.what() << std::endl;
        }
    }
}
