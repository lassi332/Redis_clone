#pragma once

#include <string>
#include <vector>
#include <utility>
#include <string_view>

enum class RespType {
    SimpleString,
    Error,
    Integer,
    BulkString,
    Array,
    Null
};

struct RespObject {
    RespType type = RespType::Null;
    std::string str_val;
    long long int_val = 0;
    std::vector<RespObject> array_val;

    std::string serialize() const;
};

// Main parser function
// Returns: {Parsed RespObject, bytes consumed}
// If incomplete, returns {RespObject with type Null, 0}
std::pair<RespObject, size_t> parse_resp(const std::string_view& data);

// Quick helper constructors
RespObject make_simple_string(const std::string& val);
RespObject make_error(const std::string& val);
RespObject make_integer(long long val);
RespObject make_bulk_string(const std::string& val);
RespObject make_null();
