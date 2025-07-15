#include <string>
#include <optional>
#include <iostream>

#include "ExprGenerator.h"
#include "ConnectionsHandler.h"

struct CommandLineArgs {
    int n;
    int connections;
    std::string server_addr;
    int server_port;
};

std::pair<CommandLineArgs, std::optional<std::string>> parse_cla(int argc, char *argv[]) {
    if (argc != 5) {
        return {{}, ("Usage: " + std::string(argv[0]) + " <n> <connections> <server_addr> <server_port>")};
    }

    int n = std::atoi(argv[1]);
    int connections = std::atoi(argv[2]);
    std::string server_addr = argv[3];
    int server_port = std::atoi(argv[4]);

    if (n <= 0 || connections <= 0 || server_port <= 0) {
        return {{}, {"Invalid arguments"}};
    }

    return {CommandLineArgs{
        .n = n,
        .connections = connections,
        .server_addr = std::move(server_addr),
        .server_port = server_port
    }, std::nullopt};
}

void receive_callback(ConnectionsHandler::Context const &ctx, std::string data) {
    long srv_res = std::stol(data);
    long loc_res = ExprGenerator::evaluate_check(ctx.expr);
    if (srv_res != loc_res) {
        std::cerr << "Expr: " << ctx.expr << " Server: " << srv_res << " Expected: " << loc_res << std::endl;
    } else {
        // std::cout << "Check passed!" << std::endl;
    }
}

int main(int argc, char *argv[]) {
    auto const &[args, err] = parse_cla(argc, argv);

    if (err.has_value()) {
        std::cerr << err.value() << '\n';
        return EXIT_FAILURE;
    }

    ConnectionsHandler connections_handler(args.server_addr, args.server_port, args.n);
    ExprGenerator expr_generator{};

    for (int i = 0; i < args.connections; ++i) {
        connections_handler.create_new_connection({expr_generator.gen_expr(args.n)});
    }

    std::cout << "Sending expressions..." << std::endl;

    connections_handler.send_all(receive_callback);

    std::cout << "Sending ended" << std::endl;
    return 0;
}
