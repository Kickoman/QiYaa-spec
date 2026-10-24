#include "vectors.h"

#include "spotify/protocol/access_point.h"
#include "spotify/protocol/crypto.h"
#include "spotify/protocol/protobuf.h"
#include "spotify/protocol/shannon.h"

#include <algorithm>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace Vectors {

namespace {

using SpotifyProtocol::AccessPoint;
using SpotifyProtocol::ProtoReader;
using SpotifyProtocol::ProtoWriter;

constexpr char kDhPrime[] =
    "ffffffffffffffffc90fdaa22168c234c4c6628b80dc1cd129024e088a67cc74020bbea63b139b22514a08798e34"
    "04ddef9519b3cd3a431b302b0a6df25f14374fe1356d6d51c245e485b576625e7ec6f44c42e9a63a3620ffffffff"
    "ffffffff";
constexpr char kServerPrivate[] =
    "030a11181f262d343b424950575e656c737a81888f969da4abb2b9c0c7ced5dce3eaf1f8ff060d141b222930373e"
    "454c535a61686f767d848b9299a0a7aeb5bcc3cad1d8dfe6edf4fb020910171e252c333a41484f565d646b727980"
    "878e95";
constexpr char kServerPublic[] =
    "d15d5ac9ee0a866599a2aaaceeb7f1ec6caa1d2e0420c2fc3a8b2b81ed46632109be52227c4c0216866af715977c"
    "e95a139c8890c511e2eff06400dc9f946c23aac8287f1a3e847c804802449ac74d090498818f1cf318eb22315c73"
    "8de00134";
constexpr char kTestKeyModulus[] =
    "bc84542f4bc0d900043c734b883eafd51fab91e634e6cb27be036da84505da2701ddf842c5dc37d9d13df0457e35"
    "6951e9e3a1e3dfb7abcb7f501ac52454438e8bf38b84c7b180f7e9f82cfb26c421024cb674856b5552c9e14a35a3"
    "c35d32cd5512c9cdbc977bafb06692d4298d9af55606e29d3b9774b8cd3a2732fee997e240ddfc0ec599cc07b2c1"
    "d9e6eb605a05d11cb85a87cfba9f4f157cb646840ddffd5e07ee24172f9c3eb6959d2e61ba43a7bc2fbc52489ff9"
    "fad09b26cdda39b4c9175d77d0539b2b3ee9dd0691b76401f463a593002283eb67d6fed14b76172b13c7c813dbff"
    "23c8254cf368a34165099b934de13a31d416c248e0568113148b";
constexpr char kServerSignature[] =
    "16473ab622628535ee8c0864c1969307527c309b099bf17dffff55c4444a28aa8d611b847427425ff884cde4f793"
    "d0b2ae226d90ca81efe7273a079cddec0d48eca3cd33ff04fe957145c93dd9450d224c763b0b548b4bd764112ea2"
    "2eea261ee0dd58633be46164f1daecff4827c1f7c11b5d248c93d97ee9eb65a14ce87b0fc1032d34e01c3f49916f"
    "b3c62290d16aaac0c4f79a944a3b433a0139c027160470365ce040494aff0ad0ffde882fb5ca13baa7c468f48e72"
    "f3175ff75f19d26461bb314e50cff88d147c33770854633b6d914e597d298dbcb39740464be63db36b0c4c19eb61"
    "8aa5570f9b373e1cd95e89d534a5f38629bc8f9f89de9a0ed6c0";

constexpr std::uint8_t kPing = 0x04;
constexpr std::uint8_t kRequestKey = 0x0c;
constexpr std::uint8_t kAesKey = 0x0d;
constexpr std::uint8_t kAesKeyError = 0x0e;
constexpr std::uint8_t kCountryCode = 0x1b;
constexpr std::uint8_t kPong = 0x49;
constexpr std::uint8_t kPongAck = 0x4a;
constexpr std::uint8_t kProductInfo = 0x50;
constexpr std::uint8_t kLicenseVersion = 0x76;
constexpr std::uint8_t kLogin = 0xab;
constexpr std::uint8_t kApWelcome = 0xac;
constexpr std::uint8_t kAuthFailure = 0xad;

const char* CommandName(std::uint8_t command) {
    switch (command) {
        case kPing: return "Ping";
        case kRequestKey: return "RequestKey";
        case kAesKey: return "AesKey";
        case kAesKeyError: return "AesKeyError";
        case kCountryCode: return "CountryCode";
        case kPong: return "Pong";
        case kPongAck: return "PongAck";
        case kProductInfo: return "ProductInfo";
        case kLicenseVersion: return "LicenseVersion";
        case kLogin: return "Login";
        case kApWelcome: return "APWelcome";
        case kAuthFailure: return "AuthFailure";
        default: return "unknown";
    }
}

ByteView FieldOf(ByteView message, int field) {
    ProtoReader reader(message);
    while (reader.next()) {
        if (reader.field() == field) {
            return reader.bytes();
        }
    }
    return {};
}

struct Message {
    std::string what;
    TBytes wire;
    std::optional<std::uint32_t> nonce;
    std::uint8_t command = 0;
    TBytes payload;
};

Json MessageJson(const Message& message) {
    Json json = Json::Object();
    json.set("what", message.what);
    if (message.nonce) {
        json.set("nonce", *message.nonce).set("command", Hex(TBytes{message.command})).set("payload", Hex(message.payload));
    }
    return json.set("wire", Hex(message.wire));
}

Json MessagesJson(const std::vector<Message>& messages) {
    Json json = Json::Array();
    for (const Message& message : messages) {
        json.push(MessageJson(message));
    }
    return json;
}

Json EventJson(const SpotifyProtocol::TApEvent& event) {
    return std::visit(
        [](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            Json json = Json::Object();
            if constexpr (std::is_same_v<T, SpotifyProtocol::ApLoggedIn>) {
                json.set("event", "loggedIn")
                    .set("username", value.reusable.username)
                    .set("type", static_cast<unsigned>(value.reusable.type))
                    .set("data", Hex(value.reusable.data));
            } else if constexpr (std::is_same_v<T, SpotifyProtocol::ApLoginFailed>) {
                json.set("event", "loginFailed")
                    .set("code", static_cast<unsigned>(value.code))
                    .set("description", value.description);
            } else if constexpr (std::is_same_v<T, SpotifyProtocol::ApCountry>) {
                json.set("event", "country").set("code", value.code);
            } else if constexpr (std::is_same_v<T, SpotifyProtocol::ApProduct>) {
                json.set("event", "product").set("type", value.type);
            } else if constexpr (std::is_same_v<T, SpotifyProtocol::ApAudioKey>) {
                json.set("event", "audioKey").set("sequence", value.sequence).set("key", Hex(value.key));
            } else if constexpr (std::is_same_v<T, SpotifyProtocol::ApAudioKeyRefused>) {
                json.set("event", "audioKeyRefused").set("sequence", value.sequence).set("code", value.code);
            } else {
                json.set("event", "failure");
            }
            return json;
        },
        event);
}

class FakeAccessPoint {
public:
    enum class Reply { Challenge, TryAnotherAp };

    explicit FakeAccessPoint(Reply kind)
        : reply(kind) { }

    std::vector<Message> read(const TBytes& bytes) {
        incoming.insert(incoming.end(), bytes.begin(), bytes.end());
        std::vector<Message> messages;
        while (std::optional<Message> message = next()) {
            messages.push_back(std::move(*message));
        }
        Check(incoming.empty(), "the fake access point left bytes unread");
        return messages;
    }

    Message send(std::uint8_t command, const TBytes& payload) {
        TBytes packet{command, static_cast<std::uint8_t>(payload.size() >> 8), static_cast<std::uint8_t>(payload.size())};
        packet.insert(packet.end(), payload.begin(), payload.end());
        const std::uint32_t nonce = sendNonce++;
        sendCipher->nonce(nonce);
        sendCipher->encrypt(packet.data(), packet.size());
        std::uint8_t mac[4];
        sendCipher->finish(mac, sizeof mac);
        packet.insert(packet.end(), mac, mac + sizeof mac);
        return {CommandName(command), packet, nonce, command, payload};
    }

    std::optional<Message> response;
    TBytes clientPublic;
    TBytes shared;
    TBytes material;
    bool answered = false;

private:
    enum class Stage { Hello, Answer, Packets };

    std::optional<Message> next() {
        switch (stage) {
            case Stage::Hello: return hello();
            case Stage::Answer: return answer();
            case Stage::Packets: return packet();
        }
        return std::nullopt;
    }

    std::optional<Message> hello() {
        if (incoming.size() < 6) {
            return std::nullopt;
        }
        const std::uint32_t size = SpotifyProtocol::ReadU32(incoming.data() + 2);
        Check(incoming.size() >= size, "a whole client hello");
        const TBytes wire = Slice(incoming, 0, size);
        incoming.erase(incoming.begin(), incoming.begin() + size);
        transcript = wire;
        const ByteView hello(wire.data() + 6, size - 6);
        clientPublic = FieldOf(FieldOf(FieldOf(hello, 50), 10), 10).copy();

        ProtoWriter body;
        if (reply == Reply::TryAnotherAp) {
            ProtoWriter failed;
            failed.varint(10, static_cast<std::uint64_t>(SpotifyProtocol::LoginError::TryAnotherAp));
            body.message(30, failed);
        } else {
            ProtoWriter dh;
            dh.bytes(10, Bytes(kServerPublic)).varint(20, 1).bytes(30, Bytes(kServerSignature));
            ProtoWriter crypto;
            crypto.message(10, dh);
            ProtoWriter challenge;
            challenge.message(10, crypto);
            body.message(10, challenge);
        }
        TBytes replyWire;
        SpotifyProtocol::AppendU32(replyWire, static_cast<std::uint32_t>(4 + body.data().size()));
        replyWire.insert(replyWire.end(), body.data().begin(), body.data().end());
        transcript.insert(transcript.end(), replyWire.begin(), replyWire.end());
        response = Message{"APResponseMessage", replyWire, std::nullopt, 0, {}};

        const std::optional<TBytes> secret =
            SpotifyProtocol::ModPow(clientPublic, Bytes(kServerPrivate), Bytes(kDhPrime));
        Check(secret.has_value(), "the fake access point's shared secret");
        shared = StripLeadingZeros(*secret);
        for (std::uint8_t i = 1; i < 6; ++i) {
            const SpotifyProtocol::TSha1 part = SpotifyProtocol::HmacSha1(shared, {transcript, {&i, 1}});
            material.insert(material.end(), part.begin(), part.end());
        }
        receiveCipher.emplace(ByteView{material.data() + 20, 32});
        sendCipher.emplace(ByteView{material.data() + 52, 32});
        stage = Stage::Answer;
        return Message{"ClientHello", wire, std::nullopt, 0, {}};
    }

    std::optional<Message> answer() {
        if (incoming.size() < 4) {
            return std::nullopt;
        }
        const std::uint32_t size = SpotifyProtocol::ReadU32(incoming.data());
        Check(incoming.size() >= size, "a whole challenge answer");
        const TBytes wire = Slice(incoming, 0, size);
        incoming.erase(incoming.begin(), incoming.begin() + size);
        const ByteView body(wire.data() + 4, size - 4);
        const TBytes hmac = FieldOf(FieldOf(FieldOf(body, 10), 10), 10).copy();
        const SpotifyProtocol::TSha1 expected = SpotifyProtocol::HmacSha1({material.data(), 20}, {transcript});
        answered = hmac == TBytes(expected.begin(), expected.end());
        stage = Stage::Packets;
        return Message{"ClientResponsePlaintext", wire, std::nullopt, 0, {}};
    }

    std::optional<Message> packet() {
        if (incoming.empty()) {
            return std::nullopt;
        }
        Check(incoming.size() >= 7, "a whole packet header");
        TBytes header = Slice(incoming, 0, 3);
        SpotifyProtocol::Shannon probe = *receiveCipher;
        probe.nonce(receiveNonce);
        probe.decrypt(header.data(), header.size());
        const std::size_t size = SpotifyProtocol::ReadU16(header.data() + 1);
        Check(incoming.size() >= 3 + size + 4, "a whole packet");
        const TBytes wire = Slice(incoming, 0, 3 + size + 4);
        incoming.erase(incoming.begin(), incoming.begin() + static_cast<std::ptrdiff_t>(wire.size()));
        TBytes plain = Slice(wire, 0, 3 + size);
        const std::uint32_t nonce = receiveNonce++;
        receiveCipher->nonce(nonce);
        receiveCipher->decrypt(plain.data(), plain.size());
        std::uint8_t mac[4];
        receiveCipher->finish(mac, sizeof mac);
        Check(std::equal(mac, mac + 4, wire.begin() + static_cast<std::ptrdiff_t>(3 + size)), "the client's MAC");
        return Message{CommandName(plain[0]), wire, nonce, plain[0], Slice(plain, 3, size)};
    }

    Reply reply;
    Stage stage = Stage::Hello;
    TBytes incoming;
    TBytes transcript;
    std::optional<SpotifyProtocol::Shannon> sendCipher;
    std::optional<SpotifyProtocol::Shannon> receiveCipher;
    std::uint32_t sendNonce = 0;
    std::uint32_t receiveNonce = 0;
};

SpotifyProtocol::ClientIdentity Identity() {
    SpotifyProtocol::ClientIdentity identity;
    identity.systemInformation = "QiYaa 0.7.2";
    identity.versionString = "QiYaa 0.7.2";
    identity.deviceId = std::string(40, 'd');
    return identity;
}

SpotifyProtocol::Credentials Stored() {
    SpotifyProtocol::Credentials credentials;
    credentials.username = "owner";
    credentials.type = SpotifyProtocol::AuthenticationType::StoredCredentials;
    credentials.data = TBytes{1, 2, 3, 4};
    return credentials;
}

SpotifyProtocol::ServerKey TestKey() {
    SpotifyProtocol::ServerKey key;
    key.modulus = Bytes(kTestKeyModulus);
    key.exponent = 65537;
    return key;
}

struct Randomness {
    unsigned step = 13;
    unsigned start = 5;
};

class Session {
public:
    Session(const std::string& name, SpotifyProtocol::Credentials login, Randomness pattern,
            SpotifyProtocol::ServerKey trusted, FakeAccessPoint::Reply reply)
        : credentials(login)
        , trustedKey(trusted)
        , client(Identity(), login, [this, pattern](std::uint8_t* data, std::size_t size) {
            const TBytes bytes = Counting(size, pattern.step, pattern.start);
            std::copy(bytes.begin(), bytes.end(), data);
            random.push_back(bytes);
        }, trusted)
        , server(reply) {
        json.set("name", name);
    }

    void start() {
        client.start();
        record(Json::Object().set("call", "start"), {});
    }

    void receive(std::int64_t nowMs, const std::vector<Message>& incoming) {
        TBytes bytes;
        for (const Message& message : incoming) {
            bytes.insert(bytes.end(), message.wire.begin(), message.wire.end());
        }
        client.receive(bytes, nowMs);
        record(Json::Object().set("call", "receive").set("nowMs", nowMs), incoming);
    }

    void advance(std::int64_t nowMs) {
        client.advance(nowMs);
        record(Json::Object().set("call", "advance").set("nowMs", nowMs), {});
    }

    std::optional<std::uint32_t> requestAudioKey(const SpotifyProtocol::TGid& track, const SpotifyProtocol::TFileId& file) {
        const std::optional<std::uint32_t> sequence = client.requestAudioKey(track, file);
        Json step = Json::Object();
        step.set("call", "requestAudioKey").set("track", Hex(track)).set("file", Hex(file));
        step.set("returns", sequence ? Json(*sequence) : Json());
        record(std::move(step), {});
        return sequence;
    }

    Message response() const {
        Check(server.response.has_value(), "the access point's response");
        return *server.response;
    }

    Json done() {
        const SpotifyProtocol::ClientIdentity identity = Identity();
        json.set("identity",
                 Json::Object()
                     .set("platform", static_cast<unsigned>(identity.platform))
                     .set("cpuFamily", static_cast<unsigned>(identity.cpuFamily))
                     .set("os", static_cast<unsigned>(identity.os))
                     .set("protocolVersion", identity.protocolVersion)
                     .set("systemInformation", identity.systemInformation)
                     .set("versionString", identity.versionString)
                     .set("deviceId", identity.deviceId));
        json.set("credentials",
                 Json::Object()
                     .set("username", credentials.username)
                     .set("type", static_cast<unsigned>(credentials.type))
                     .set("data", Hex(credentials.data)));
        Json given = Json::Array();
        for (const TBytes& bytes : random) {
            given.push(Hex(bytes));
        }
        json.set("random", std::move(given));
        json.set("trustedKey", Json::Object().set("modulus", Hex(trustedKey.modulus)).set("exponent", trustedKey.exponent));
        json.set("dhPrime", kDhPrime);
        json.set("server",
                 Json::Object()
                     .set("dhPrivate", kServerPrivate)
                     .set("dhPublic", kServerPublic)
                     .set("rsaModulus", kTestKeyModulus)
                     .set("rsaExponent", 65537)
                     .set("signature", kServerSignature));
        if (server.answered) {
            json.set("keys",
                     Json::Object()
                         .set("clientPublic", Hex(server.clientPublic))
                         .set("shared", Hex(server.shared))
                         .set("material", Hex(server.material))
                         .set("challengeAnswer", Hex(Slice(answerOf(), 0, 20)))
                         .set("clientToServer", Hex(Slice(server.material, 20, 32)))
                         .set("serverToClient", Hex(Slice(server.material, 52, 32))));
        } else {
            json.set("keys", Json());
        }
        json.set("steps", std::move(steps));
        return std::move(json);
    }

private:
    TBytes answerOf() const {
        for (const Message& message : sent) {
            if (message.what == "ClientResponsePlaintext") {
                const ByteView body(message.wire.data() + 4, message.wire.size() - 4);
                return FieldOf(FieldOf(FieldOf(body, 10), 10), 10).copy();
            }
        }
        Check(false, "no challenge answer");
        return {};
    }

    void record(Json step, const std::vector<Message>& incoming) {
        const std::vector<Message> outgoing = server.read(client.takeOutgoing());
        sent.insert(sent.end(), outgoing.begin(), outgoing.end());
        if (!incoming.empty()) {
            step.set("incoming", MessagesJson(incoming));
        }
        step.set("outgoing", MessagesJson(outgoing));
        Json events = Json::Array();
        for (const SpotifyProtocol::TApEvent& event : client.takeEvents()) {
            events.push(EventJson(event));
        }
        const std::optional<std::int64_t> deadline = client.deadline();
        step.set("events", std::move(events))
            .set("deadline", deadline ? Json(*deadline) : Json())
            .set("loggedIn", client.loggedIn())
            .set("failed", client.failed());
        steps.push(std::move(step));
    }

    SpotifyProtocol::Credentials credentials;
    SpotifyProtocol::ServerKey trustedKey;
    std::vector<TBytes> random;
    AccessPoint client;
    Json json = Json::Object();
    Json steps = Json::Array();

public:
    FakeAccessPoint server;
    std::vector<Message> sent;
};

TBytes U32(std::uint32_t value) {
    TBytes bytes;
    SpotifyProtocol::AppendU32(bytes, value);
    return bytes;
}

TBytes Welcome(const std::string& username, const TBytes& blob) {
    ProtoWriter message;
    message.string(10, username).varint(20, 1).varint(25, 1).varint(30, 1).bytes(40, blob);
    return message.data();
}

void LogIn(Session& session, std::int64_t nowMs) {
    session.start();
    session.receive(0, {session.response()});
    Check(session.server.answered, "the client answers the challenge");
    session.receive(nowMs, {session.server.send(kApWelcome, Welcome("owner", {9, 9, 9}))});
}

Json FullSession() {
    Session session("stored credentials: login, country, product, two audio keys, the keep-alive", Stored(), {},
                    TestKey(), FakeAccessPoint::Reply::Challenge);
    session.start();
    session.receive(0, {session.response()});
    Check(session.server.answered, "the client answers the challenge");
    const std::string product = "<products><product><type>premium</type></product></products>";
    session.receive(1000, {
                              session.server.send(kApWelcome, Welcome("owner", {9, 9, 9})),
                              session.server.send(kCountryCode, Text("BY")),
                              session.server.send(kProductInfo, Text(product)),
                              session.server.send(kLicenseVersion, {0, 0}),
                          });

    SpotifyProtocol::TGid track{};
    track[0] = 1;
    SpotifyProtocol::TFileId file{};
    file[0] = 2;
    const std::optional<std::uint32_t> first = session.requestAudioKey(track, file);
    Check(first == 0u, "the first key request");
    session.receive(2000, {session.server.send(kAesKey, Join(U32(*first), TBytes(16, 0x77)))});
    const std::optional<std::uint32_t> second = session.requestAudioKey(track, file);
    Check(second == 1u, "the second key request");
    session.receive(3000, {session.server.send(kAesKeyError, Join(U32(*second), {0, 1}))});

    session.receive(5000, {session.server.send(kPing, {0, 0, 0, 1})});
    session.advance(5000 + AccessPoint::kPongDelayMs - 1);
    const std::int64_t pongAt = 5000 + AccessPoint::kPongDelayMs;
    session.advance(pongAt);
    session.receive(pongAt + 100, {session.server.send(kPongAck, {})});
    session.advance(pongAt + 100 + AccessPoint::kNextPingMs);
    return session.done();
}

Json TokenRefused() {
    Session session("an OAuth token: the login is refused", SpotifyProtocol::Credentials::FromAccessToken("BQ-token"),
                    {}, TestKey(), FakeAccessPoint::Reply::Challenge);
    session.start();
    session.receive(0, {session.response()});
    ProtoWriter failure;
    failure.varint(10, static_cast<std::uint64_t>(SpotifyProtocol::LoginError::BadCredentials))
        .string(40, "bad credentials");
    session.receive(1000, {session.server.send(kAuthFailure, failure.data())});
    const std::optional<std::uint32_t> refused = session.requestAudioKey({}, {});
    Check(!refused, "no audio key after a refused login");
    return session.done();
}

Json HandshakeRefused() {
    Session session("the handshake is refused: TryAnotherAP", Stored(), {}, TestKey(),
                    FakeAccessPoint::Reply::TryAnotherAp);
    session.start();
    session.receive(0, {session.response()});
    return session.done();
}

Json NotSigned() {
    Session session("an access point not signed by the trusted key", Stored(), {}, SpotifyProtocol::SpotifyServerKey(),
                    FakeAccessPoint::Reply::Challenge);
    session.start();
    session.receive(0, {session.response()});
    return session.done();
}

Randomness FindRandomness(bool publicKeyZero) {
    const TBytes prime = Bytes(kDhPrime);
    for (unsigned start = 6; start < 256; ++start) {
        const TBytes privateKey = Counting(95, 13, start);
        const std::optional<TBytes> clientPublic = SpotifyProtocol::ModPow(TBytes{2}, privateKey, prime);
        const std::optional<TBytes> shared = SpotifyProtocol::ModPow(Bytes(kServerPublic), privateKey, prime);
        Check(clientPublic && shared, "DH for the search");
        const bool publicZero = (*clientPublic)[0] == 0;
        const bool sharedZero = (*shared)[0] == 0;
        if (publicKeyZero ? publicZero && !sharedZero : sharedZero && !publicZero) {
            return {13, start};
        }
    }
    for (unsigned step = 15; step < 64; step += 2) {
        for (unsigned start = 0; start < 256; ++start) {
            const TBytes privateKey = Counting(95, step, start);
            const std::optional<TBytes> clientPublic = SpotifyProtocol::ModPow(TBytes{2}, privateKey, prime);
            const std::optional<TBytes> shared = SpotifyProtocol::ModPow(Bytes(kServerPublic), privateKey, prime);
            const bool publicZero = (*clientPublic)[0] == 0;
            const bool sharedZero = (*shared)[0] == 0;
            if (publicKeyZero ? publicZero && !sharedZero : sharedZero && !publicZero) {
                return {step, start};
            }
        }
    }
    Check(false, "no randomness with a leading zero");
    return {};
}

Json LeadingZero(const std::string& name, bool publicKeyZero) {
    Session session(name, Stored(), FindRandomness(publicKeyZero), TestKey(), FakeAccessPoint::Reply::Challenge);
    LogIn(session, 1000);
    Check(session.server.answered, "the client answers the challenge");
    if (publicKeyZero) {
        Check(session.server.clientPublic.size() < 96, "the client public key loses its leading zero");
    } else {
        Check(session.server.shared.size() < 96, "the shared secret loses its leading zero");
    }
    return session.done();
}

}  // namespace

Json HandshakeVectors() {
    Check(Bytes(kServerPrivate) == Counting(95, 7, 3), "the fake access point's private key");
    Check(SpotifyProtocol::ModPow(TBytes{2}, Bytes(kServerPrivate), Bytes(kDhPrime)) == Bytes(kServerPublic),
          "the fake access point's public key");
    Check(SpotifyProtocol::VerifyRsaSha1(Bytes(kTestKeyModulus), 65537, SpotifyProtocol::Sha1({Bytes(kServerPublic)}),
                                         Bytes(kServerSignature)),
          "the fake access point's signature");

    Json cases = Json::Array();
    cases.push(FullSession());
    cases.push(TokenRefused());
    cases.push(HandshakeRefused());
    cases.push(NotSigned());
    cases.push(LeadingZero("the client's public key has a leading zero byte", true));
    cases.push(LeadingZero("the shared secret has a leading zero byte", false));
    return Json::Object()
        .set("description",
             "Access point sessions against a fake access point: the handshake, the login, packets both ways, the "
             "audio keys and the keep-alive, with the client's randomness fixed.")
        .set("cases", std::move(cases));
}

}  // namespace Vectors
