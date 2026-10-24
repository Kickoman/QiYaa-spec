#include "vectors.h"

#include "spotify/protocol/catalog.h"
#include "spotify/protocol/crypto.h"
#include "spotify/protocol/ids.h"
#include "spotify/protocol/login.h"
#include "spotify/protocol/metadata.h"
#include "spotify/protocol/protobuf.h"

#include <algorithm>
#include <chrono>
#include <optional>
#include <utility>
#include <vector>

namespace Vectors {

namespace {

using SpotifyProtocol::ProtoReader;
using SpotifyProtocol::ProtoWriter;

constexpr char kClientId[] = "65b708073fc0480ea92a077233ca87bd";
constexpr char kClientVersion[] = "1.2.52.442";
const std::string kDeviceId(40, 'd');

struct Field {
    enum class Kind { Varint, String, Bytes, Message };

    int number = 0;
    Kind kind = Kind::Varint;
    std::uint64_t varint = 0;
    std::string text;
    TBytes bytes;
    std::vector<Field> children;
};

Field V(int number, std::uint64_t value) {
    Field field;
    field.number = number;
    field.varint = value;
    return field;
}

Field S(int number, std::string text) {
    Field field;
    field.number = number;
    field.kind = Field::Kind::String;
    field.text = std::move(text);
    return field;
}

Field B(int number, TBytes bytes) {
    Field field;
    field.number = number;
    field.kind = Field::Kind::Bytes;
    field.bytes = std::move(bytes);
    return field;
}

Field M(int number, std::vector<Field> children) {
    Field field;
    field.number = number;
    field.kind = Field::Kind::Message;
    field.children = std::move(children);
    return field;
}

ProtoWriter Write(const std::vector<Field>& fields) {
    ProtoWriter writer;
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

Json FieldsJson(const std::vector<Field>& fields) {
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

struct ReadResult {
    Json fields = Json::Array();
    bool failed = false;
};

ReadResult Read(const TBytes& bytes) {
    ReadResult result;
    ProtoReader reader(bytes);
    while (reader.next()) {
        Json item = Json::Object();
        item.set("field", reader.field()).set("wire", static_cast<int>(reader.type()));
        switch (reader.type()) {
            case SpotifyProtocol::ProtoWire::Varint:
                item.set("varint", std::to_string(reader.varint())).set("zigzag", std::to_string(reader.zigzag()));
                break;
            case SpotifyProtocol::ProtoWire::Fixed64:
            case SpotifyProtocol::ProtoWire::Fixed32:
                item.set("bytes", Hex(reader.bytes())).set("value", std::to_string(reader.varint()));
                break;
            case SpotifyProtocol::ProtoWire::Bytes: item.set("bytes", Hex(reader.bytes())); break;
        }
        result.fields.push(std::move(item));
    }
    result.failed = reader.failed();
    return result;
}

std::string NextBase62(std::string text) {
    static const std::string digits = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (std::size_t i = text.size(); i > 0; --i) {
        const std::size_t digit = digits.find(text[i - 1]);
        if (digit + 1 < digits.size()) {
            text[i - 1] = digits[digit + 1];
            return text;
        }
        text[i - 1] = '0';
    }
    Check(false, "base62 overflow");
    return text;
}

SpotifyProtocol::HashcashSolution Solve(const TBytes& context, const TBytes& prefix, int length) {
    SpotifyProtocol::HashcashChallenge challenge;
    challenge.prefix = prefix;
    challenge.length = length;
    const std::optional<SpotifyProtocol::HashcashSolution> solution = SpotifyProtocol::SolveHashcash(context, challenge);
    Check(solution.has_value(), "hashcash solved in time");
    return *solution;
}

}  // namespace

Json ProtobufVectors() {
    Json cases = Json::Array();
    const auto write = [&](const std::string& name, const std::vector<Field>& fields) {
        const TBytes bytes = Write(fields).data();
        cases.push(Json::Object()
                       .set("kind", "write")
                       .set("name", name)
                       .set("fields", FieldsJson(fields))
                       .set("bytes", Hex(bytes)));
        return bytes;
    };
    const auto read = [&](const std::string& name, const TBytes& bytes) {
        ReadResult result = Read(bytes);
        Check(!result.failed, "protobuf read " + name);
        cases.push(Json::Object()
                       .set("kind", "read")
                       .set("name", name)
                       .set("bytes", Hex(bytes))
                       .set("fields", std::move(result.fields)));
    };
    const auto reject = [&](const std::string& name, const TBytes& bytes) {
        ReadResult result = Read(bytes);
        Check(result.failed, "protobuf reject " + name);
        cases.push(Json::Object()
                       .set("kind", "reject")
                       .set("name", name)
                       .set("bytes", Hex(bytes))
                       .set("fieldsBefore", std::move(result.fields)));
    };

    const std::vector<Field> inner{V(1, 300), S(2, "name")};
    Check(Hex(write("a varint and a string", inner)) == "08ac0212046e616d65", "protobuf inner message");
    const TBytes outer = write("a nested message", {M(10, inner), V(100, 1)});
    std::vector<Field> varints;
    for (const std::uint64_t value :
         {std::uint64_t{0}, std::uint64_t{1}, std::uint64_t{127}, std::uint64_t{128}, std::uint64_t{16383},
          std::uint64_t{16384}, (std::uint64_t{1} << 21) - 1, std::uint64_t{1} << 21, (std::uint64_t{1} << 28) - 1,
          std::uint64_t{1} << 28, (std::uint64_t{1} << 32) - 1, std::uint64_t{1} << 35, std::uint64_t{1} << 42,
          std::uint64_t{1} << 49, std::uint64_t{1} << 56, (std::uint64_t{1} << 63) - 1, std::uint64_t{1} << 63,
          ~std::uint64_t{0}}) {
        varints.push_back(V(1, value));
    }
    write("varints of every length", varints);
    std::vector<Field> numbers;
    for (const int number : {1, 15, 16, 2047, 2048, 262143, 262144, 33554431, 33554432, 536870911}) {
        numbers.push_back(V(number, 1));
    }
    write("field numbers with tags of one to five bytes", numbers);
    write("an empty message, empty bytes and an empty string", {M(20, {}), B(30, {}), S(40, "")});
    write("130 bytes take a two-byte length", {B(3, Counting(130, 1, 0))});
    write("a string is written as its UTF-8 bytes", {S(2, "Песня — Ø")});
    write("fields stay in the order written, repeated ones too", {S(3, "a"), V(1, 1), S(3, "b")});

    read("a nested message is a length-delimited field", outer);
    read("fixed32 and fixed64 are little-endian", Bytes("0d01020304" "110102030405060708"));
    {
        std::vector<Field> zigzag;
        for (const std::uint64_t value : {0ull, 1ull, 2ull, 3ull, 445092ull, 4294967294ull, 4294967295ull}) {
            zigzag.push_back(V(7, value));
        }
        read("varints as zigzag: 0, -1, 1, -2, 222546, 2^31 - 1, -2^31", Write(zigzag).data());
    }
    read("the largest varint", Bytes("08" "ffffffffffffffffff01"));
    read("the largest field number", Write({V(536870911, 0)}).data());
    read("an empty length-delimited field", Bytes("0a00"));
    read("a varint in more bytes than needed", Bytes("088000"));

    reject("a length past the end", Bytes("0a0568656c"));
    reject("a varint cut short", Bytes("08ff"));
    reject("a tag cut short after a whole field", Bytes("0801ff"));
    reject("a group start (wire type 3)", Bytes("0b"));
    reject("a group end (wire type 4)", Bytes("0801" "0c"));
    reject("wire type 6", Bytes("0e00"));
    reject("wire type 7", Bytes("0f00"));
    reject("field number 0", Bytes("0001"));
    reject("a field number above 2^29 - 1", Bytes("808080801000"));
    reject("fixed32 cut short", Bytes("0d0102"));
    reject("fixed64 cut short", Bytes("0901020304050607"));
    reject("a varint of more than ten bytes", Bytes("08" "ffffffffffffffffffff01"));
    reject("a length of 2^64 - 1", Bytes("0a" "ffffffffffffffffff01" "00"));

    return Json::Object()
        .set("description",
             "The protobuf wire format the Spotify protocol code writes and reads: fields to bytes, bytes to the "
             "fields a reader sees, and bytes a reader must reject.")
        .set("cases", std::move(cases));
}

Json IdVectors() {
    Json cases = Json::Array();
    const auto valid = [&](const std::string& name, const std::string& base62, const std::string& gidHex) {
        const std::optional<SpotifyProtocol::TGid> gid = SpotifyProtocol::GidFromBase62(base62);
        Check(gid.has_value() && Hex(*gid) == gidHex, "base62 " + base62);
        Check(SpotifyProtocol::Base62FromGid(*gid) == base62, "base62 round trip " + base62);
        cases.push(Json::Object()
                       .set("name", name)
                       .set("base62", base62)
                       .set("gid", gidHex)
                       .set("uri", "spotify:track:" + base62));
    };
    const auto fromGid = [&](const std::string& name, const std::string& gidHex) {
        SpotifyProtocol::TGid gid{};
        const TBytes bytes = Bytes(gidHex);
        Check(bytes.size() == gid.size(), "gid size");
        std::copy(bytes.begin(), bytes.end(), gid.begin());
        valid(name, SpotifyProtocol::Base62FromGid(gid), gidHex);
    };
    const auto invalid = [&](const std::string& name, const std::string& base62) {
        Check(!SpotifyProtocol::GidFromBase62(base62).has_value(), "base62 refused " + base62);
        cases.push(Json::Object().set("name", name).set("base62", base62).set("gid", Json()).set("uri", Json()));
    };

    valid("a track id", "6PCoZdjSPbRMehHviTrpB3", Hex(*SpotifyProtocol::GidFromBase62("6PCoZdjSPbRMehHviTrpB3")));
    valid("one", "0000000000000000000001", "00000000000000000000000000000001");
    fromGid("zero", "00000000000000000000000000000000");
    fromGid("10 is a: the digits are 0-9, a-z, A-Z", "0000000000000000000000000000000a");
    fromGid("36 is A", "00000000000000000000000000000024");
    fromGid("61 is Z", "0000000000000000000000000000003d");
    fromGid("62 is 10", "0000000000000000000000000000003e");
    fromGid("the track of handshake.json's key requests", "01000000000000000000000000000000");
    fromGid("the largest gid", "ffffffffffffffffffffffffffffffff");

    SpotifyProtocol::TGid largest{};
    largest.fill(0xff);
    invalid("too short", "tooShort");
    invalid("23 characters", "6PCoZdjSPbRMehHviTrpB3a");
    invalid("a character outside base62", "6PCoZdjSPbRMehHviTrp-3");
    invalid("one above the largest gid", NextBase62(SpotifyProtocol::Base62FromGid(largest)));
    invalid("the largest 22 base62 digits", std::string(22, 'Z'));

    return Json::Object()
        .set("description",
             "Spotify ids: a 22-character base62 id and its 16-byte gid, both ways, and the track URI made from it.")
        .set("cases", std::move(cases));
}

Json MessageVectors() {
    Json cases = Json::Array();
    const SpotifyProtocol::Credentials credentials{"owner", SpotifyProtocol::AuthenticationType::StoredCredentials,
                                                   TBytes{1, 2, 3, 4}};

    const auto hashcash = [&](const std::string& name, const TBytes& context, const TBytes& prefix, int length) {
        const SpotifyProtocol::HashcashSolution solution = Solve(context, prefix, length);
        const SpotifyProtocol::TSha1 digest = SpotifyProtocol::Sha1({prefix, solution.suffix});
        std::uint64_t tail = 0;
        for (std::size_t i = 12; i < 20; ++i) {
            tail = tail << 8 | digest[i];
        }
        Check(length == 0 || (tail & ((std::uint64_t{1} << length) - 1)) == 0, "hashcash zeros " + name);
        cases.push(Json::Object()
                       .set("kind", "hashcash")
                       .set("name", name)
                       .set("context", Hex(context))
                       .set("prefix", Hex(prefix))
                       .set("length", length)
                       .set("suffix", Hex(solution.suffix)));
        return solution.suffix;
    };
    const auto suffix = hashcash("client-token: no context", {}, Bytes("0123456789abcdef"), 10);
    const TBytes loginContext = Text("synthetic login5 context");
    const auto loginSuffix = hashcash("login5: with the reply's context", loginContext, Counting(16, 17, 3), 14);
    hashcash("length 0: the first suffix", Text("ctx"), Counting(16, 1, 0), 0);

    const auto login5 = [&](const std::string& name, const TBytes& context, std::optional<std::int64_t> tookNanos) {
        std::optional<SpotifyProtocol::HashcashSolution> solution;
        Json hashcashJson;
        if (tookNanos) {
            solution = SpotifyProtocol::HashcashSolution{};
            solution->suffix = loginSuffix;
            solution->took = std::chrono::nanoseconds(*tookNanos);
            hashcashJson = Json::Object().set("suffix", Hex(loginSuffix)).set("tookNanos", *tookNanos);
        }
        const TBytes body = SpotifyProtocol::Login5Request(kClientId, kDeviceId, credentials, context, solution);
        cases.push(Json::Object()
                       .set("kind", "login5")
                       .set("name", name)
                       .set("clientId", kClientId)
                       .set("deviceId", kDeviceId)
                       .set("username", credentials.username)
                       .set("data", Hex(credentials.data))
                       .set("context", Hex(context))
                       .set("hashcash", std::move(hashcashJson))
                       .set("body", Hex(body)));
    };
    login5("stored credentials, the first request", {}, std::nullopt);
    login5("the answer to a hashcash challenge that took 1.5 s", loginContext, 1'500'000'000);
    login5("a solution under a second: no seconds", loginContext, 12'345'678);
    login5("a solution of exactly 2 s: no nanoseconds", loginContext, 2'000'000'000);
    login5("a solution of 0 ns: an empty duration", loginContext, 0);

    const auto clientToken = [&](const std::string& name, const SpotifyProtocol::TPlatformData& platform,
                                 Json platformJson) {
        SpotifyProtocol::ClientTokenParams params;
        params.clientVersion = kClientVersion;
        params.clientId = kClientId;
        params.deviceId = kDeviceId;
        params.platform = platform;
        cases.push(Json::Object()
                       .set("kind", "clientToken")
                       .set("name", name)
                       .set("clientVersion", kClientVersion)
                       .set("clientId", kClientId)
                       .set("deviceId", kDeviceId)
                       .set("platform", std::move(platformJson))
                       .set("body", Hex(SpotifyProtocol::ClientTokenRequest(params))));
    };
    const auto linuxPlatform = [&](const std::string& name, const std::string& release, const std::string& version,
                                   const std::string& hardware) {
        clientToken(name, SpotifyProtocol::LinuxPlatform{release, version, hardware},
                    Json::Object()
                        .set("os", "linux")
                        .set("release", release)
                        .set("version", version)
                        .set("hardware", hardware));
    };
    linuxPlatform("Linux on x86_64", "6.8.0-45-generic", "24.04", "x86_64");
    linuxPlatform("Android, as Linux on ARM", "5.15.148-android14-11", "14", "aarch64");
    {
        const SpotifyProtocol::WindowsPlatform windows;
        clientToken("Windows on x86_64 (desktop only)", windows,
                    Json::Object()
                        .set("os", "windows")
                        .set("osVersion", windows.osVersion)
                        .set("osBuild", windows.osBuild)
                        .set("imageFileMachine", windows.imageFileMachine)
                        .set("peMachine", windows.peMachine));
    }
    clientToken("macOS (desktop only)", SpotifyProtocol::MacPlatform{"15.4", "arm64"},
                Json::Object().set("os", "macos").set("systemVersion", "15.4").set("cpuType", "arm64"));

    {
        SpotifyProtocol::HashcashSolution solution;
        solution.suffix = suffix;
        cases.push(Json::Object()
                       .set("kind", "clientTokenAnswer")
                       .set("name", "the answer to client-token's hashcash: the suffix in upper-case hex")
                       .set("state", "synthetic-state")
                       .set("suffix", Hex(suffix))
                       .set("body", Hex(SpotifyProtocol::ClientTokenAnswer("synthetic-state", solution))));
    }

    const auto metadata = [&](const std::string& name, const std::vector<std::string>& uris,
                              SpotifyProtocol::ExtensionKind kind) {
        Json list = Json::Array();
        for (const std::string& uri : uris) {
            list.push(uri);
        }
        cases.push(Json::Object()
                       .set("kind", "metadata")
                       .set("name", name)
                       .set("uris", std::move(list))
                       .set("extensionKind", static_cast<unsigned>(kind))
                       .set("body", Hex(SpotifyProtocol::MetadataRequest(uris, kind))));
    };
    metadata("one track: TRACK_V4", {"spotify:track:6PCoZdjSPbRMehHviTrpB3"}, SpotifyProtocol::ExtensionKind::TrackV4);
    metadata("two albums at once: ALBUM_V4", {"spotify:album:0000000000000000000001", "spotify:album:0000000000000000000002"},
             SpotifyProtocol::ExtensionKind::AlbumV4);
    metadata("an artist: ARTIST_V4", {"spotify:artist:0000000000000000000003"}, SpotifyProtocol::ExtensionKind::ArtistV4);
    Check(SpotifyProtocol::TrackMetadataRequest("spotify:track:6PCoZdjSPbRMehHviTrpB3") ==
              SpotifyProtocol::MetadataRequest({"spotify:track:6PCoZdjSPbRMehHviTrpB3"},
                                               SpotifyProtocol::ExtensionKind::TrackV4),
          "track metadata request");

    const auto autoplay = [&](const std::string& name, const std::string& context, const std::vector<std::string>& recent) {
        Json list = Json::Array();
        for (const std::string& uri : recent) {
            list.push(uri);
        }
        cases.push(Json::Object()
                       .set("kind", "autoplay")
                       .set("name", name)
                       .set("contextUri", context)
                       .set("recentTrackUris", std::move(list))
                       .set("body", Hex(SpotifyProtocol::AutoplayRequest(context, recent))));
    };
    autoplay("liked songs with two recent tracks", "spotify:user:owner:collection",
             {"spotify:track:0000000000000000000001", "spotify:track:0000000000000000000002"});
    autoplay("a track, no recent tracks", "spotify:track:6PCoZdjSPbRMehHviTrpB3", {});

    const auto collection = [&](const std::string& name, const std::string& set, const std::string& token, int limit) {
        cases.push(Json::Object()
                       .set("kind", "collection")
                       .set("name", name)
                       .set("username", "owner")
                       .set("set", set)
                       .set("pageToken", token)
                       .set("limit", limit)
                       .set("body", Hex(SpotifyProtocol::CollectionPageRequest("owner", set, token, limit))));
    };
    collection("the first page of liked songs and saved albums", "collection", "", 300);
    collection("the next page", "collection", "synthetic-page-token", 300);
    collection("followed artists", "artist", "", 50);

    return Json::Object()
        .set("description",
             "Request bodies the Spotify protocol code builds: login5, client-token and its hashcash answer, "
             "extended-metadata, autoplay and the collection, and hashcash solutions.")
        .set("cases", std::move(cases));
}

}  // namespace Vectors
