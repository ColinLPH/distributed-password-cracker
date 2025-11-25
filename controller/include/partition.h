#ifndef PARTITION_H
#define PARTITION_H

#include <atomic>
#include <cmath>
#include <string>
#include <vector>
#include <memory>
#include "password.h"

constexpr size_t DEFAULT_PREFIX_LEN = 1;
constexpr size_t DEFAULT_DEPTH = 3;
constexpr size_t DEFAULT_BRANCHING = 1;

enum PartitionProgress
{
    READY = 0,
    IN_PROGRESS,
    COMPLETED
};

struct Partition
{
    std::string prefix;
    std::atomic<PartitionProgress> status;

    Partition(const std::string &pfx) : prefix(pfx), status(READY) {}
};

std::vector<std::string> generate_prefixes(const unsigned prefix_len);
std::vector<std::unique_ptr<Partition>> create_partitions(const unsigned prefix_len);

#endif // PARTITION_H
