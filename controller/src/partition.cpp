#include "partition.h"

std::vector<std::string> generate_prefixes(const unsigned prefix_len)
{
    // For simplicity, only generating single-character prefixes
    // Extend this function to generate multi-character prefixes as needed
    std::vector<std::string> prefixes;

    for (size_t i = 0; i < sizeof(CHAR_SET)-1; ++i) {
        prefixes.push_back(std::string(1, CHAR_SET[i]));
    }
    return prefixes;
}

std::vector<std::unique_ptr<Partition>> create_partitions(const unsigned prefix_len)
{
    auto prefixes = generate_prefixes(prefix_len);
    std::vector<std::unique_ptr<Partition>> partitions;
    for (const auto& pfx : prefixes) {
        partitions.emplace_back(std::make_unique<Partition>(pfx));
    }
    return partitions;
}
