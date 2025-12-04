#include "parse_args.h"

void print_args(const Args &args)
{
    std::cout << "Server IP: " << args.serverIP << "\n";
    std::cout << "Server Port: " << args.server_port << "\n";
    std::cout << "Worker Thread Count: " << args.threads << "\n";
}

int parse_args(int argc, char *argv[], Args &args)
{
    static struct option long_options[] = {
        {"server", required_argument, 0, 's'},
        {"port", required_argument, 0, 'p'},
        {"threads", required_argument, 0, 't'},
        {0, 0, 0, 0}};

    if (argc != 7)
    {
        std::cerr << "Usage: " << argv[0] << " [--server serverIP] [--port server_port] [--threads num_threads]\n";
        return -1;
    }

    int option_index = 0;
    int opt;
    while ((opt = getopt_long(argc, argv, "s:p:t:", long_options, &option_index)) != -1)
    {
        try
        {
            switch (opt)
            {
            case 's':
                args.serverIP = optarg;
                if (args.serverIP.empty())
                {
                    throw std::invalid_argument("Server IP cannot be empty");
                }
                break;
            case 'p':
                args.server_port = std::stoi(optarg);
                if (args.server_port < 1 || args.server_port > 65535)
                {
                    throw std::out_of_range("Port number must be between 1 and 65535");
                }
                if (args.server_port < 1024)
                {
                    std::cerr << "Warning: Using a port number below 1024 may require elevated privileges.\n";
                }
                break;
            case 't':
                args.threads = std::stoi(optarg);
                if (args.threads < 1)
                {
                    throw std::out_of_range("Number of threads must be at least 1");
                }
                break;
            case '?':
                throw std::invalid_argument(
                    "Invalid option: Usage: " + std::string(argv[0]) +
                    " [--server serverIP] [--port server_port] [--threads num_threads]");
            default:
                throw std::invalid_argument("Unexpected error parsing options");
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << "\n";
            return -1; // clean failure, no crash
        }
    }

    return 0;
}
