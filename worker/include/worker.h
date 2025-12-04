#ifndef WORKER_H
#define WORKER_H

#include <atomic>
#include <crypt.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <memory>
#include <iostream>
#include <mutex>

constexpr int MIN_HASH_TOKENS = 3;
constexpr int MAX_HASH_TOKENS = 4;

// character set for generating password candidates
const char CHAR_SET[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789"
    "@#%^&*()_+-=.,:;?";  

constexpr auto CHAR_SET_SIZE = sizeof(CHAR_SET) - 1;

struct hash_info {
    std::string algorithm;
    std::string options;
    std::string salt;
    std::string hash;
    std::string full_hash;
};

void print_hash_info(const hash_info& info);
hash_info parse_hash_info(const std::string& hash_field);
std::vector<std::string> split(const std::string& str, char delim);

std::string generate_hash(const std::string& password_candidate, 
                          const hash_info& hashData);
std::string generate_salt_for_hash(const hash_info& hashData);

void generate_combination(std::string &starter);
void update_total_work_done(std::shared_ptr<std::atomic<uint16_t>> &total_work_done, 
                            const uint16_t work_size, 
                            std::shared_ptr<std::atomic<bool>> &work_completed);

#endif // WORKER_H
