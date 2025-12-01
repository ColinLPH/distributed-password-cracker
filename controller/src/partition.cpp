#include "partition.h"

std::vector<std::string> generate_prefixes(const unsigned prefix_len)
{
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

std::vector<std::string> generate_work_prefixes(const std::vector<std::unique_ptr<Partition>> &partitions, 
    size_t &part_index, uint8_t num_prefixes)
{
    std::vector<std::string> work_prefixes;
    for(uint8_t i = 0; i < num_prefixes; ++i) {
        work_prefixes.push_back(partitions[part_index]->prefix);
        partitions[part_index]->status.store(IN_PROGRESS);
        part_index = (part_index + 1) % partitions.size();
    }

    return work_prefixes;
}

int update_prefix(std::string &prefix, std::vector<std::unique_ptr<Partition>> &partitions)
{
    for (auto& part : partitions) {
        if (prefix[0] == part->prefix[0]) {
            std::cout << "Updating prefix: '" << part->prefix << "' to '" << prefix << "'\n";
            part->prefix = prefix;
            return 0;
        }
    }
    return -1;
}
