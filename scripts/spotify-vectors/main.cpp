#include "vectors.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <utility>

namespace Vectors {

void Check(bool condition, const std::string& what) {
    if (!condition) {
        std::cerr << "spotify-vectors: check failed: " << what << '\n';
        std::exit(1);
    }
}

TBytes Bytes(std::string_view hex) {
    const std::optional<TBytes> bytes = SpotifyProtocol::FromHex(hex);
    Check(bytes.has_value(), "bad hex constant");
    return *bytes;
}

TBytes Text(std::string_view text) {
    return ByteView::Of(text).copy();
}

TBytes Counting(std::size_t size, unsigned step, unsigned start) {
    TBytes bytes(size);
    for (std::size_t i = 0; i < size; ++i) {
        bytes[i] = static_cast<std::uint8_t>(i * step + start);
    }
    return bytes;
}

TBytes StripLeadingZeros(TBytes bytes) {
    std::size_t zeros = 0;
    while (zeros < bytes.size() && bytes[zeros] == 0) {
        ++zeros;
    }
    bytes.erase(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(zeros));
    return bytes;
}

std::string Hex(ByteView bytes) {
    return SpotifyProtocol::ToHex(bytes);
}

TBytes Slice(const TBytes& bytes, std::size_t offset, std::size_t size) {
    Check(offset + size <= bytes.size(), "slice out of range");
    return TBytes(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                  bytes.begin() + static_cast<std::ptrdiff_t>(offset + size));
}

TBytes Join(const TBytes& first, const TBytes& second) {
    TBytes joined = first;
    joined.insert(joined.end(), second.begin(), second.end());
    return joined;
}

}  // namespace Vectors

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: spotify-vectors <directory to write the spotify/*.json files into>\n";
        return 2;
    }
    const std::string directory = argv[1];
    const std::pair<const char*, Vectors::Json (*)()> files[] = {
        {"shannon.json", Vectors::ShannonVectors},
        {"crypto.json", Vectors::CryptoVectors},
        {"audio.json", Vectors::AudioVectors},
        {"handshake.json", Vectors::HandshakeVectors},
        {"protobuf.json", Vectors::ProtobufVectors},
        {"ids.json", Vectors::IdVectors},
        {"messages.json", Vectors::MessageVectors},
    };
    for (const auto& [name, make] : files) {
        const std::string text = make().dump();
        const std::string path = directory + "/" + name;
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << text;
        out.close();
        Vectors::Check(static_cast<bool>(out), "could not write " + path);
        std::cout << path << ": " << text.size() << " bytes\n";
    }
    return 0;
}
