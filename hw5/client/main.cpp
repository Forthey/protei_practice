#include <string>
#include <optional>
#include <iostream>

#include "ExprGenerator.h"
#include "ConnectionsHandler.h"
#include "utility/random_int.h"

struct CommandLineArgs {
    int const n;
    int const connections;
    std::string const server_addr;
    int const server_port;
    int const max_expr_in_req;
};

std::pair<CommandLineArgs, std::optional<std::string>> parse_cla(int argc, char *argv[]) {
    if (argc < 5 || argc > 6) {
        return {{}, ("Usage: " + std::string(argv[0]) + " <n> <connections> <server_addr> <server_port> <max_expr_in_req>")};
    }

    int n = std::atoi(argv[1]);
    int connections = std::atoi(argv[2]);
    std::string server_addr = argv[3];
    int server_port = std::atoi(argv[4]);
    int max_expr_in_req = argc == 6 ? std::atoi(argv[5]) : 1;

    if (n <= 0 || connections <= 0 || server_port <= 0 || max_expr_in_req <= 0) {
        return {{}, {"Invalid arguments"}};
    }

    return {CommandLineArgs{
        .n = n,
        .connections = connections,
        .server_addr = std::move(server_addr),
        .server_port = server_port,
        .max_expr_in_req = max_expr_in_req
    }, std::nullopt};
}

void receive_callback(ConnectionsHandler::Context const &ctx, std::string data) {
    std::istringstream results_stream(data), expressions_stream(data);

    std::string expression, result;
    while (expressions_stream >> expression) {
        if (!(results_stream >> result)) {
            std::cerr << "Expr and Results size mismatch" << std::endl;
            return;
        }
        try {
            long srv_res = std::stol(result);
            long loc_res = ExprGenerator::evaluate_check(expression);
            if (srv_res != loc_res) {
                std::cerr << "Expr: " << ctx.expressions << " Server: " << srv_res << " Expected: " << loc_res << std::endl;
            } else {
                // std::cout << "Check passed!" << std::endl;
            }
        } catch (std::invalid_argument const &e) {
            std::cerr << e.what() << std::endl;
            std::cerr << result.data() << std::endl;
        }
    }
}

int main(int argc, char *argv[]) {
    auto const &[args, err] = parse_cla(argc, argv);

    if (err.has_value()) {
        std::cerr << err.value() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "Establishing " << args.connections << " connections..." << std::endl;

    ConnectionsHandler connections_handler(args.server_addr, args.server_port, args.n);
    ExprGenerator expr_generator{};

    std::cout << "Generating expressions..." << std::endl;

    std::size_t total_sent = 0;

    for (int i = 0; i < args.connections; ++i) {
        std::ostringstream oss;
        std::size_t expr_cnt = random_int(1, args.max_expr_in_req);
        for (std::size_t j = 0; j < expr_cnt; ++j) {
            oss << expr_generator.gen_expr(args.n);
            if (j != expr_cnt - 1) {
                oss << ' ';
            }
        }
        connections_handler.create_new_connection({.expressions = oss.str()});
        total_sent += expr_cnt;
    }

    std::cout << "Sending " << total_sent << " expressions..." << std::endl;
    connections_handler.send_all(receive_callback);
    std::cout << "Sending ended" << std::endl;
    return 0;
}
