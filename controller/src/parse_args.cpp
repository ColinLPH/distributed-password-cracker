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
                    if (args.port < 1 || args.port > 65535) {
                        throw std::out_of_range("Port number must be between 1 and 65535");
                    }
                    if (args.port < 1024) {
                        std::cerr << "Warning: Using a port number below 1024 may require elevated privileges.\n";
                    }
                    break;
                case 'w':
                    args.work_size = std::stoi(optarg);
                    if (args.work_size <= 0) {
                        throw std::out_of_range("Work size must be a positive integer");
                    }
                    break;
                case 'c':
                    args.checkpoint_interval = std::stoi(optarg);
                    if (args.checkpoint_interval <= 0) {
                        throw std::out_of_range("Checkpoint interval must be a positive integer");
                    }
                    break;
                case 't':
                    args.timeout = std::stoi(optarg);
                    if (args.timeout <= 0) {
                        throw std::out_of_range("Timeout must be a positive integer");
                    }
                    break;
                case 'h':
                    if(!optarg || std::string(optarg).empty()) {
                        throw std::invalid_argument("Hash string cannot be empty");
                    }
                    if(!is_valid_hash(std::string(optarg))) {
                        throw std::invalid_argument("Invalid hash format");
                    }
                    args.hash = optarg;
                    break;
                case '?': 
                    throw std::invalid_argument(
                        "Invalid option: Usage: " + std::string(argv[0]) +
                        " [--port port] [--work-size work_size] [--checkpoint checkpoint_interval] [--timeout timeout] [--hash hash]");
                default:
                    throw std::invalid_argument("Unexpected error parsing options");
            }
        }         
        catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << "\n";
            return -1;   // clean failure, no crash
        }
    }

    return 0;

}

#include <regex>
#include <string>

int is_valid_hash(const std::string &hash)
{
    static const std::regex md5_regex(
        "^[A-Fa-f0-9]{32}$"
    );
    
    static const std::regex bcrypt_regex(
        R"(^\$2[aby]\$([0-2][0-9]|3[0-1])\$[./A-Za-z0-9]{53}$)"
    );

    static const std::regex yescrypt_regex(
        R"(^\$(y|7)\$[^\$]+\$[./A-Za-z0-9]+\$[./A-Za-z0-9]+$)"
    );

    static const std::regex sha256crypt_regex(
        R"(^\$5\$[A-Za-z0-9./]{1,16}\$[A-Za-z0-9./]{43}$)"
    );

    static const std::regex sha512crypt_regex(
        R"(^\$6\$[A-Za-z0-9./]{1,16}\$[A-Za-z0-9./]{86}$)"
    );

    if (std::regex_match(hash, md5_regex))        return 1;
    if (std::regex_match(hash, sha256crypt_regex)) return 1;
    if (std::regex_match(hash, sha512crypt_regex)) return 1;
    if (std::regex_match(hash, bcrypt_regex))     return 1;
    if (std::regex_match(hash, yescrypt_regex))   return 1;

    return 0;
}

