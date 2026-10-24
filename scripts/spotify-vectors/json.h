#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace Vectors {

class Json {
public:
    Json()
        : type(Type::Null) { }
    Json(bool value)
        : type(Type::Bool)
        , flag(value) { }
    Json(int value)
        : type(Type::Integer)
        , text(std::to_string(value)) { }
    Json(long value)
        : type(Type::Integer)
        , text(std::to_string(value)) { }
    Json(long long value)
        : type(Type::Integer)
        , text(std::to_string(value)) { }
    Json(unsigned value)
        : type(Type::Integer)
        , text(std::to_string(value)) { }
    Json(unsigned long value)
        : type(Type::Integer)
        , text(std::to_string(value)) { }
    Json(unsigned long long value)
        : type(Type::Integer)
        , text(std::to_string(value)) { }
    Json(const char* value)
        : type(Type::String)
        , text(value) { }
    Json(std::string value)
        : type(Type::String)
        , text(std::move(value)) { }

    static Json Object() {
        Json json;
        json.type = Type::Object;
        return json;
    }

    static Json Array() {
        Json json;
        json.type = Type::Array;
        return json;
    }

    Json& set(std::string key, Json value) {
        keys.push_back(std::move(key));
        items.push_back(std::move(value));
        return *this;
    }

    Json& push(Json value) {
        items.push_back(std::move(value));
        return *this;
    }

    std::string dump() const {
        std::string out;
        write(out, 0);
        out.push_back('\n');
        return out;
    }

private:
    enum class Type { Null, Bool, Integer, String, Array, Object };

    static void Quote(const std::string& value, std::string& out) {
        out.push_back('"');
        for (const char c : value) {
            const auto byte = static_cast<unsigned char>(c);
            switch (c) {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (byte < 0x20) {
                        char escaped[8];
                        std::snprintf(escaped, sizeof escaped, "\\u%04x", byte);
                        out += escaped;
                    } else {
                        out.push_back(c);
                    }
            }
        }
        out.push_back('"');
    }

    void write(std::string& out, int depth) const {
        switch (type) {
            case Type::Null: out += "null"; return;
            case Type::Bool: out += flag ? "true" : "false"; return;
            case Type::Integer: out += text; return;
            case Type::String: Quote(text, out); return;
            case Type::Array:
            case Type::Object: break;
        }
        const bool object = type == Type::Object;
        if (items.empty()) {
            out += object ? "{}" : "[]";
            return;
        }
        out += object ? "{\n" : "[\n";
        for (std::size_t i = 0; i < items.size(); ++i) {
            out.append(static_cast<std::size_t>(depth + 1) * 2, ' ');
            if (object) {
                Quote(keys[i], out);
                out += ": ";
            }
            items[i].write(out, depth + 1);
            out += i + 1 < items.size() ? ",\n" : "\n";
        }
        out.append(static_cast<std::size_t>(depth) * 2, ' ');
        out += object ? "}" : "]";
    }

    Type type;
    bool flag = false;
    std::string text;
    std::vector<std::string> keys;
    std::vector<Json> items;
};

}  // namespace Vectors
