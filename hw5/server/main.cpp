#include <iostream>
#include <optional>
#include <string>
#include <sstream>

#include "Calculator.h"
#include "ConnectionsHandler.h"


struct CommandLineArgs {
    int port;
};

std::pair<CommandLineArgs, std::optional<std::string>> parse_cla(int argc, char *argv[]) {
    if (argc != 2) {
        return {{}, ("Usage: " + std::string(argv[0]) + " <port>")};
    }

    int port = atoi(argv[1]);

    if (port <= 0) {
        return {{}, {"Invalid arguments"}};
    }

    return {CommandLineArgs{.port = port}, std::nullopt};
}


std::string calculate(std::string const& data) {
    std::ostringstream oss;
    std::istringstream iss(data);
    std::string token;
    bool first = true;
    while (iss >> token) {
        long res = Calculator::evaluate(token);
        if (!first) {
            oss << ' ';
        }
        oss << res;
        first = false;
    }

    return oss.str();
}

int main(int argc, char *argv[]) {
    auto const &[args, err] = parse_cla(argc, argv);

    if (err.has_value()) {
        std::cerr << err.value() << '\n';
        return EXIT_FAILURE;
    }

    ConnectionsHandler connections_handler(args.port);

    connections_handler.listen(calculate);

    return EXIT_SUCCESS;
}
