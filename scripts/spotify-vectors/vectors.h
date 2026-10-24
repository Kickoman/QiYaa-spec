#pragma once

#include "json.h"

#include "spotify/protocol/bytes.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace Vectors {

using SpotifyProtocol::ByteView;
using SpotifyProtocol::TBytes;

void Check(bool condition, const std::string& what);
TBytes Bytes(std::string_view hex);
TBytes Text(std::string_view text);
TBytes Counting(std::size_t size, unsigned step, unsigned start);
TBytes StripLeadingZeros(TBytes bytes);
std::string Hex(ByteView bytes);
TBytes Slice(const TBytes& bytes, std::size_t offset, std::size_t size);
TBytes Join(const TBytes& first, const TBytes& second);
std::string Base64(ByteView bytes);

Json ShannonVectors();
Json CryptoVectors();
Json AudioVectors();
Json HandshakeVectors();
Json ProtobufVectors();
Json IdVectors();
Json MessageVectors();

}  // namespace Vectors
