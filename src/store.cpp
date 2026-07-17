#include "store.hpp"

void Store::set(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    db_[key] = value;
}

bool Store::del(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    return db_.erase(key) > 0;
}

bool Store::get(const std::string& key, std::string& value) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = db_.find(key);
    if (it != db_.end()) {
        value = it->second;
        return true;
    }
    return false;
}

bool Store::exists(const std::string& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return db_.find(key) != db_.end();
}
