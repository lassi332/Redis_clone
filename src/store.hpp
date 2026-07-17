#pragma once

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <mutex>

class Store {
public:
    Store() = default;
    ~Store() = default;

    // Write operations (Exclusive Lock)
    void set(const std::string& key, const std::string& value);
    bool del(const std::string& key);

    // Read operations (Shared Lock)
    bool get(const std::string& key, std::string& value) const;
    bool exists(const std::string& key) const;

private:
    std::unordered_map<std::string, std::string> db_;
    mutable std::shared_mutex mutex_;
};
