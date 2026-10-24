#pragma once

#include "vectors.h"

#include "spotify/protocol/protobuf.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Vectors {

struct Field {
    enum class Kind { Varint, String, Bytes, Message };

    int number = 0;
    Kind kind = Kind::Varint;
    std::uint64_t varint = 0;
    std::string text;
    TBytes bytes;
    std::vector<Field> children;
};

inline Field V(int number, std::uint64_t value) {
    Field field;
    field.number = number;
    field.varint = value;
    return field;
}

inline Field S(int number, std::string text) {
    Field field;
    field.number = number;
    field.kind = Field::Kind::String;
    field.text = std::move(text);
    return field;
}

inline Field B(int number, TBytes bytes) {
    Field field;
    field.number = number;
    field.kind = Field::Kind::Bytes;
    field.bytes = std::move(bytes);
    return field;
}

inline Field M(int number, std::vector<Field> children) {
    Field field;
    field.number = number;
    field.kind = Field::Kind::Message;
    field.children = std::move(children);
    return field;
}

inline SpotifyProtocol::ProtoWriter Write(const std::vector<Field>& fields) {
    SpotifyProtocol::ProtoWriter writer;
    for (const Field& field : fields) {
        switch (field.kind) {
            case Field::Kind::Varint: writer.varint(field.number, field.varint); break;
            case Field::Kind::String: writer.string(field.number, field.text); break;
            case Field::Kind::Bytes: writer.bytes(field.number, field.bytes); break;
            case Field::Kind::Message: writer.message(field.number, Write(field.children)); break;
        }
    }
    return writer;
}

inline Json FieldsJson(const std::vector<Field>& fields) {
    Json json = Json::Array();
    for (const Field& field : fields) {
        Json item = Json::Object();
        item.set("field", field.number);
        switch (field.kind) {
            case Field::Kind::Varint: item.set("varint", std::to_string(field.varint)); break;
            case Field::Kind::String: item.set("string", field.text); break;
            case Field::Kind::Bytes: item.set("bytes", Hex(field.bytes)); break;
            case Field::Kind::Message: item.set("message", FieldsJson(field.children)); break;
        }
        json.push(std::move(item));
    }
    return json;
}

}  // namespace Vectors
