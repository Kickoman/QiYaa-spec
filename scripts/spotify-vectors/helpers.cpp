#include "vectors.h"

#include <cstdlib>
#include <iostream>
#include <optional>

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

std::string Base64(ByteView bytes) {
    static constexpr char kDigits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string text;
    for (std::size_t i = 0; i < bytes.size; i += 3) {
        const std::size_t left = bytes.size - i;
        std::uint32_t group = static_cast<std::uint32_t>(bytes.data[i]) << 16;
        if (left > 1) {
            group |= static_cast<std::uint32_t>(bytes.data[i + 1]) << 8;
        }
        if (left > 2) {
            group |= bytes.data[i + 2];
        }
        text.push_back(kDigits[group >> 18 & 63]);
        text.push_back(kDigits[group >> 12 & 63]);
        text.push_back(left > 1 ? kDigits[group >> 6 & 63] : '=');
        text.push_back(left > 2 ? kDigits[group & 63] : '=');
    }
    return text;
}

}  // namespace Vectors
