#include "worker.h"

std::mutex total_work_mutex;

hash_info parse_hash_info(const std::string &hash_field)
{
    hash_info info;

    if (hash_field.empty())
    {
        throw std::invalid_argument("Empty hash field");
    }
    // add other error checks as needed

    info.full_hash = hash_field;

    auto tokens = split(hash_field, '$');

    if (tokens.size() == MIN_HASH_TOKENS)
    {
        info.algorithm = "$" + tokens[0];
        info.options = "";
        info.salt = tokens[1];
        if (info.algorithm == "$2b" || info.algorithm == "$2a" || info.algorithm == "$2y")
        {
            info.salt += "$";
            for (size_t i = 0; i < 22; ++i)
            {
                info.salt += tokens[2][i];
            }
            info.hash = tokens[2].substr(22);
        }
        else
        {
            info.hash = tokens[2];
        }
    }
    else if (tokens.size() == MAX_HASH_TOKENS)
    {
        info.algorithm = "$" + tokens[0];
        info.options = tokens[1];
        info.salt = tokens[2];
        info.hash = tokens[3];
    }
    else
    {
        throw std::invalid_argument("Invalid hash field: " + hash_field);
    }

    return info;
}

std::vector<std::string> split(const std::string &str, char delim)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delim))
    {
        if (!token.empty())
        {
            tokens.push_back(token);
        }
    }

    return tokens;
}

void print_hash_info(const hash_info &info)
{
    std::cout << "Hash Info:\n";
    std::cout << "Full Hash: " << info.full_hash << "\n";
    std::cout << "Algorithm: " << info.algorithm << "\n";
    std::cout << "Options: " << info.options << "\n";
    std::cout << "Salt: " << info.salt << "\n";
    std::cout << "Hash: " << info.hash << "\n";
}

std::string generate_hash(const std::string &password_candidate, const hash_info &hashData)
{
    struct crypt_data data;
    data.initialized = 0;

    std::string salt = generate_salt_for_hash(hashData);
    const char *result = crypt_r(password_candidate.c_str(), salt.c_str(), &data);

    if (result == nullptr || result == "*0")
    {
        throw std::runtime_error("crypt_r failed to generate hash");
    }

    return std::string(result);
}

std::string generate_salt_for_hash(const hash_info &hashData)
{
    std::string salt = hashData.algorithm;
    if (!hashData.options.empty())
    {
        salt += "$" + hashData.options;
    }
    if (hashData.algorithm == "$2b" || hashData.algorithm == "$2a" || hashData.algorithm == "$2y")
    {
        salt += "$" + hashData.salt; // bcrypt salt includes extra data
    }
    else
    {
        salt += "$" + hashData.salt + "$";
    }
    return salt;
}

void generate_combination(std::string &starter)
{
    size_t len = starter.size();

    // RULE 1: length 1 → do nothing
    if (len == 1)
    {
        starter += CHAR_SET[0];
        return;
    }

    // Helper: find index of a char in CHAR_SET
    auto find_index = [&](char ch)
    {
        for (size_t i = 0; i < CHAR_SET_SIZE; ++i)
            if (CHAR_SET[i] == ch)
                return i;
        return (size_t)-1; // shouldn't happen
    };

    // RULE 2: length 2 → increment ONLY last character
    if (len == 2)
    {
        size_t idx = find_index(starter[1]);

        if (idx < CHAR_SET_SIZE - 1)
        {
            starter[1] = CHAR_SET[idx + 1];
        }
        else
        {
            // overflow: reset last char to CHAR_SET[0]
            starter[1] = CHAR_SET[0];
            // and append new CHAR_SET[0]
            starter += CHAR_SET[0];
        }
        return;
    }
    // RULE 3: length >= 3 → base-N increment, except starter[0] never changes
    for (int pos = len - 1; pos >= 1; --pos)
    {
        size_t idx = find_index(starter[pos]);

        if (idx < CHAR_SET_SIZE - 1)
        {
            starter[pos] = CHAR_SET[idx + 1];
            return;
        }
        else
        {
            starter[pos] = CHAR_SET[0];

            if (pos == 1)
            {
                starter += CHAR_SET[0];
                return;
            }
        }
    }
}

void update_total_work_done(std::shared_ptr<std::atomic<uint16_t>> &total_work_done, 
                            const uint16_t work_size, 
                            std::shared_ptr<std::atomic<bool>> &work_completed)
{
    if (work_completed->load()) {
        return;
    }

    std::lock_guard<std::mutex> lock(total_work_mutex);

    total_work_done->fetch_add(1, std::memory_order_relaxed);

    if (total_work_done->load(std::memory_order_relaxed) == work_size) {
        std::cout << "total work size reached, setting work_completed to true\n";
        work_completed->store(true, std::memory_order_relaxed);
    }
}
