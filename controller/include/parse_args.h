#ifndef PARSE_ARGS_H
#define PARSE_ARGS_H

#include <iostream>
#include <getopt.h> 
#include <vector>
#include <string>
#include <sstream>

constexpr int DEFAULT_PORT = 8080;
constexpr int DEFAULT_WORK_SIZE = 10000;
constexpr int DEFAULT_CHECKPOINT_INTERVAL = 500; 
constexpr int DEFAULT_TIMEOUT = 60; 

struct Args {
    int port                = DEFAULT_PORT; 
    int work_size           = DEFAULT_WORK_SIZE; 
    int checkpoint_interval = DEFAULT_CHECKPOINT_INTERVAL; 
    int timeout             = DEFAULT_TIMEOUT; 
    std::string hash;
};

void print_args(const Args &args);
int parse_args(int argc, char *argv[], Args &args);

#endif // PARSE_ARGS_H
