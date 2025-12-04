#ifndef PARSE_ARGS_H
#define PARSE_ARGS_H

#include <iostream>
#include <getopt.h> 
#include <vector>
#include <string>
#include <sstream>
#include <regex>

constexpr int DEFAULT_PORT = 8080;
constexpr int DEFAULT_WORK_SIZE = 10000;
constexpr int DEFAULT_CHECKPOINT_INTERVAL = 500; 
constexpr int DEFAULT_TIMEOUT = 60; 
const std::string DEFAULT_HASH_SIX = "$6$Ks6ZfrXQARwpF3aH$6KBhLiqD1WNWz9/hStVgGzRj1zzTw6DZgkebDP2GR7JT68QLe8ZshpgYCs91ZMDBl9KfI4hyqiv2ppXnBWt4o1";

struct Args {
    int port                = DEFAULT_PORT; 
    int work_size           = DEFAULT_WORK_SIZE; 
    int checkpoint_interval = DEFAULT_CHECKPOINT_INTERVAL; 
    int timeout             = DEFAULT_TIMEOUT; 
    std::string hash        = DEFAULT_HASH_SIX;
};

void print_args(const Args &args);
int parse_args(int argc, char *argv[], Args &args);
int is_valid_hash(const std::string &hash);

#endif // PARSE_ARGS_H
