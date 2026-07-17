#include "resp.hpp"
#include <sstream>
#include <stdexcept>
#include <iostream>

// Helper to find the CRLF (\r\n) sequence in string_view
static size_t find_crlf(const std::string_view& data, size_t start_pos = 0) {
    return data.find("\r\n", start_pos);
}

// Helper to parse integer from a string_view
static std::pair<long long, size_t> parse_int_line(const std::string_view& data) {
    size_t crlf = find_crlf(data);
    if (crlf == std::string_view::npos) {
        return {0, 0}; // Incomplete line
    }
    std::string val_str(data.substr(0, crlf));
    try {
        long long val = std::stoll(val_str);
        return {val, crlf + 2}; // Value and bytes consumed (including CRLF)
    } catch (...) {
        return {0, 0}; // Invalid format, treat as incomplete or error
    }
}

std::string RespObject::serialize() const {
    std::ostringstream out;
    switch (type) {
        case RespType::SimpleString:
            out << "+" << str_val << "\r\n";
            break;
        case RespType::Error:
            out << "-" << str_val << "\r\n";
            break;
        case RespType::Integer:
            out << ":" << int_val << "\r\n";
            break;
        case RespType::BulkString:
            out << "$" << str_val.length() << "\r\n" << str_val << "\r\n";
            break;
        case RespType::Array:
            out << "*" << array_val.size() << "\r\n";
            for (const auto& item : array_val) {
                out << item.serialize();
            }
            break;
        case RespType::Null:
            out << "$-1\r\n";
            break;
    }
    return out.str();
}

std::pair<RespObject, size_t> parse_resp(const std::string_view& data) {
    if (data.empty()) {
        return {make_null(), 0};
    }

    char prefix = data[0];
    std::string_view payload = data.substr(1);

    switch (prefix) {
        case '+': { // Simple String
            size_t crlf = find_crlf(payload);
            if (crlf == std::string_view::npos) {
                return {make_null(), 0};
            }
            return {make_simple_string(std::string(payload.substr(0, crlf))), crlf + 2}; // +1 for prefix, +1 for \r, +1 for \n is handled by payload offset
        }
        case '-': { // Error
            size_t crlf = find_crlf(payload);
            if (crlf == std::string_view::npos) {
                return {make_null(), 0};
            }
            return {make_error(std::string(payload.substr(0, crlf))), crlf + 2};
        }
        case ':': { // Integer
            auto [val, consumed] = parse_int_line(payload);
            if (consumed == 0) {
                return {make_null(), 0};
            }
            return {make_integer(val), consumed + 1};
        }
        case '$': { // Bulk String
            auto [len, consumed] = parse_int_line(payload);
            if (consumed == 0) {
                return {make_null(), 0};
            }

            if (len == -1) {
                return {make_null(), consumed + 1};
            }

            if (len < -1) {
                return {make_null(), 0}; // Invalid length
            }

            std::string_view string_data = payload.substr(consumed);
            if (string_data.length() < static_cast<size_t>(len) + 2) {
                return {make_null(), 0}; // Incomplete data
            }

            std::string str(string_data.substr(0, len));
            // Check that it ends with CRLF
            if (string_data[len] != '\r' || string_data[len + 1] != '\n') {
                return {make_null(), 0}; // Malformed protocol
            }

            return {make_bulk_string(str), consumed + 1 + len + 2};
        }
        case '*': { // Array
            auto [count, consumed] = parse_int_line(payload);
            if (consumed == 0) {
                return {make_null(), 0};
            }

            if (count == -1) {
                return {make_null(), consumed + 1};
            }

            if (count < -1) {
                return {make_null(), 0}; // Invalid count
            }

            std::vector<RespObject> array;
            size_t total_consumed = consumed + 1;
            std::string_view remaining_payload = payload.substr(consumed);

            for (long long i = 0; i < count; ++i) {
                auto [element, element_consumed] = parse_resp(remaining_payload);
                if (element_consumed == 0) {
                    return {make_null(), 0}; // Incomplete array element
                }
                array.push_back(element);
                total_consumed += element_consumed;
                remaining_payload = remaining_payload.substr(element_consumed);
            }

            RespObject arr_obj;
            arr_obj.type = RespType::Array;
            arr_obj.array_val = std::move(array);
            return {arr_obj, total_consumed};
        }
        default:
            // Non-RESP protocol command (e.g. inline commands like "PING\n")
            // For now, let's treat it as invalid or incomplete.
            return {make_null(), 0};
    }
}

RespObject make_simple_string(const std::string& val) {
    RespObject obj;
    obj.type = RespType::SimpleString;
    obj.str_val = val;
    return obj;
}

RespObject make_error(const std::string& val) {
    RespObject obj;
    obj.type = RespType::Error;
    obj.str_val = val;
    return obj;
}

RespObject make_integer(long long val) {
    RespObject obj;
    obj.type = RespType::Integer;
    obj.int_val = val;
    return obj;
}

RespObject make_bulk_string(const std::string& val) {
    RespObject obj;
    obj.type = RespType::BulkString;
    obj.str_val = val;
    return obj;
}

RespObject make_null() {
    RespObject obj;
    obj.type = RespType::Null;
    return obj;
}
