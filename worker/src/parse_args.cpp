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
        {"server",      required_argument, 0, 's'},
        {"port",        required_argument, 0, 'p'},
        {"threads",     required_argument, 0, 't'},
        {0, 0, 0, 0} 
    };

    int option_index = 0;
    int opt;
    while ((opt = getopt_long(argc, argv, "s:p:t:", long_options, &option_index)) != -1) {
        try {
            switch (opt) {
                case 's':
                    args.serverIP = optarg;
                    break;
                case 'p':
                    args.server_port = std::stoi(optarg);
                    break;
                case 't':
                    args.threads = std::stoi(optarg);
                    break;
                case '?': 
                    throw std::invalid_argument(
                        "Invalid option: Usage: " + std::string(argv[0]) +
                        " [--server serverIP] [--port server_port] [--threads num_threads]");
                default:
                    throw std::invalid_argument("Unexpected error parsing options");
            }
        } catch (const std::invalid_argument &e) {
            throw std::invalid_argument(
                std::string("Invalid value for option '") + static_cast<char>(opt) + "': " + e.what());
                return -1;
        } catch (const std::out_of_range &e) {
            throw std::out_of_range(
                std::string("Value out of range for option '") + static_cast<char>(opt) + "': " + e.what());
                return -1;
        } catch (const std::exception &e) {
            throw std::runtime_error(
                std::string("Error processing option '") + static_cast<char>(opt) + "': " + e.what());
                return -1;
        }
    }

    return 0;

}
