#include "parse_args.h"

void print_args(const Args &args)
{
    std::cout << "Port: " << args.port << "\n";
    std::cout << "Work Size: " << args.work_size << "\n";
    std::cout << "Checkpoint Interval: " << args.checkpoint_interval << "\n";
    std::cout << "Timeout: " << args.timeout << "\n";
    std::cout << "Hash: " << args.hash << "\n";
}

int parse_args(int argc, char* argv[], Args &args) {
    static struct option long_options[] = {
        {"port",        required_argument, 0, 'p'},
        {"work-size",   required_argument, 0, 'w'},
        {"checkpoint",  required_argument, 0, 'c'},
        {"timeout",     required_argument, 0, 't'},
        {"hash",        required_argument, 0, 'h'},
        {0, 0, 0, 0} 
    };

    int option_index = 0;
    int opt;
    while ((opt = getopt_long(argc, argv, "p:w:c:t:h:", long_options, &option_index)) != -1) {
        try {
            switch (opt) {
                case 'p':
                    args.port = std::stoi(optarg);
                    break;
                case 'w':
                    args.work_size = std::stoi(optarg);
                    break;
                case 'c':
                    args.checkpoint_interval = std::stoi(optarg);
                    break;
                case 't':
                    args.timeout = std::stoi(optarg);
                    break;
                case 'h':
                    args.hash = optarg;
                    break;
                case '?': 
                    throw std::invalid_argument(
                        "Invalid option: Usage: " + std::string(argv[0]) +
                        " [--port port] [--work-size work_size] [--checkpoint checkpoint_interval] [--timeout timeout] [--hash hash]");
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
        }
    }

    return 0;

}

