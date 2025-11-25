#ifndef PARSE_ARGS_H
#define PARSE_ARGS_H

#include <iostream>
#include <getopt.h> 
#include <vector>
#include <string>
#include <sstream>

struct Args {
    int server_port;
    int threads;
    std::string serverIP;
};

void print_args(const Args &args);
int parse_args(int argc, char *argv[], Args &args);

#endif // PARSE_ARGS_H
