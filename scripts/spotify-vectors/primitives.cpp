#include "vectors.h"

#include "spotify/protocol/audio_file.h"
#include "spotify/protocol/crypto.h"
#include "spotify/protocol/shannon.h"

#include <algorithm>
#include <array>
#include <optional>
#include <utility>

namespace Vectors {

namespace {

using SpotifyProtocol::AesCtr;
using SpotifyProtocol::TAesKey;

constexpr char kDhPrime[] =
    "ffffffffffffffffc90fdaa22168c234c4c6628b80dc1cd129024e088a67cc74020bbea63b139b22514a08798e3404dd"
    "ef9519b3cd3a431b302b0a6df25f14374fe1356d6d51c245e485b576625e7ec6f44c42e9a63a3620ffffffffffffffff";

constexpr char kDesktopRsaModulus[] =
    "b2b1db1fb64016e7dbd118224e634e09efeb0368b00a236a315ebc4c341d41c852996075f0055b5ec124621575"
    "8f9ae3e2ab8994f686cb3eae1896b1606eb89eaf8d034270fcc9e4a2e04c299480c524c4f4c30d1c53e1a4e0a2"
    "dd154b75a4b52deeb9d95c5ca22c3a4e4561450e6facfabdbd30ea919b9ce1317322ee1f1723";
constexpr char kDesktopRsaSignature[] =
    "219cd729834f6b39182a7403a7325ba061d85b34558fef39b2ec95ff453b67f74e37ab903499422d8ab1d3e0c"
    "78917748783ed01dcb48c493ddcdc2237cfbc1319d6fbf49981c5f22515ce5968271e3c761a62b245289c741"
    "8b6657f8958f4a7117cdd6768a0f6a1353cfe3a90f04c25ca54e7cb203cc4fb41fe0c5a0cbe8ede";

constexpr char kSpecRsaModulus[] =
    "d4cb966609aa300f34366dc881ecfa3d466940a8711f2b1d16d5d60ff4312dec2bc9a67974e0213ba6297a9370abbcd0"
    "58853fc7fe3fa412464ae3ce4a37ecda06a3a30eb987692a861bfd1777eae3b2197b49520217132f83e0018bea0eab1a"
    "04fb82705160f31b42ed778aac94e5d0b81cccd97fb58a42ed81671ecf38818f";
constexpr char kSpecRsaPrivateExponent[] =
    "a89edb02e84dafdf9c64350259d24b491aee748a0579c95437b022fc69683f17926b2c3bba5d90780fe7767637261f6c"
    "8978ad5df11ec508a622d9f9b1de3713eb5f49b6cb3b329607688a6580c8d9d4d03ab28b9146ab0126f838d2db29849c"
    "7511d346a202a7c5f28dfecd980a5190df12f97253bc1072e7014c682c653521";

constexpr char kDigestInfoWithNull[] = "3021300906052b0e03021a05000414";
constexpr char kDigestInfoWithoutNull[] = "301f300706052b0e03021a0414";

constexpr char kAudioIv[] = "72e067fbddcbcf77ebe8bc643f630d93";

class ShannonCase {
public:
    ShannonCase(const std::string& name, const TBytes& key)
        : cipher(key) {
        json.set("name", name).set("key", Hex(key));
    }

    ShannonCase& nonce(std::uint32_t value) {
        cipher.nonce(value);
        steps.push(Json::Object().set("call", "nonce").set("value", value));
        return *this;
    }

    TBytes encrypt(const TBytes& input) {
        TBytes output = input;
        cipher.encrypt(output.data(), output.size());
        steps.push(Json::Object().set("call", "encrypt").set("input", Hex(input)).set("output", Hex(output)));
        return output;
    }

    TBytes decrypt(const TBytes& input) {
        TBytes output = input;
        cipher.decrypt(output.data(), output.size());
        steps.push(Json::Object().set("call", "decrypt").set("input", Hex(input)).set("output", Hex(output)));
        return output;
    }

    TBytes finish() {
        TBytes mac(4);
        cipher.finish(mac.data(), mac.size());
        steps.push(Json::Object().set("call", "finish").set("mac", Hex(mac)));
        return mac;
    }

    Json done() {
        json.set("steps", std::move(steps));
        return std::move(json);
    }

private:
    SpotifyProtocol::Shannon cipher;
    Json json = Json::Object();
    Json steps = Json::Array();
};

TAesKey AesKey(const TBytes& bytes) {
    Check(bytes.size() == 16, "AES key size");
    TAesKey key{};
    std::copy(bytes.begin(), bytes.end(), key.begin());
    return key;
}

std::array<std::uint8_t, 16> Block(const TBytes& bytes) {
    Check(bytes.size() == 16, "counter size");
    std::array<std::uint8_t, 16> block{};
    std::copy(bytes.begin(), bytes.end(), block.begin());
    return block;
}

TBytes Ctr(const TBytes& key, const TBytes& iv, std::uint64_t offset, TBytes data) {
    const AesCtr cipher(AesKey(key), Block(iv));
    cipher.apply(data.data(), data.size(), offset);
    return data;
}

TBytes CounterPlus(const TBytes& iv, std::uint64_t blocks) {
    TBytes counter = iv;
    std::uint64_t carry = blocks;
    for (std::size_t i = counter.size(); i > 0 && carry != 0; --i) {
        carry += counter[i - 1];
        counter[i - 1] = static_cast<std::uint8_t>(carry);
        carry >>= 8;
    }
    return counter;
}

TBytes KeystreamBlockByBlock(const TBytes& key, const TBytes& iv, std::uint64_t offset, std::size_t size) {
    TBytes stream;
    for (std::uint64_t block = offset / 16; stream.size() < size + offset % 16; ++block) {
        const TBytes one = Ctr(key, CounterPlus(iv, block), 0, TBytes(16, 0));
        stream.insert(stream.end(), one.begin(), one.end());
    }
    return Slice(stream, static_cast<std::size_t>(offset % 16), size);
}

void CheckCtrAgainstBlocks(const TBytes& key, const TBytes& iv, std::uint64_t offset, const TBytes& input,
                           const TBytes& output) {
    const TBytes stream = KeystreamBlockByBlock(key, iv, offset, input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        Check((input[i] ^ stream[i]) == output[i], "AES-CTR differs from the block-by-block counter");
    }
}

Json CtrCase(const std::string& name, const TBytes& key, const TBytes& iv, std::uint64_t offset, const TBytes& input) {
    const TBytes output = Ctr(key, iv, offset, input);
    CheckCtrAgainstBlocks(key, iv, offset, input, output);
    return Json::Object()
        .set("kind", "aes-128-ctr")
        .set("name", name)
        .set("key", Hex(key))
        .set("iv", Hex(iv))
        .set("offset", offset)
        .set("input", Hex(input))
        .set("output", Hex(output));
}

std::optional<TBytes> ModPow(const TBytes& base, const TBytes& exponent, const TBytes& modulus) {
    return SpotifyProtocol::ModPow(base, exponent, modulus);
}

Json ModPowCase(const std::string& name, const TBytes& base, const TBytes& exponent, const TBytes& modulus) {
    const std::optional<TBytes> result = ModPow(base, exponent, modulus);
    return Json::Object()
        .set("kind", "modpow")
        .set("name", name)
        .set("base", Hex(base))
        .set("exponent", Hex(exponent))
        .set("modulus", Hex(modulus))
        .set("result", result ? Json(Hex(*result)) : Json());
}

Json DhCase(const std::string& name, const TBytes& privateA, const TBytes& privateB) {
    const TBytes prime = Bytes(kDhPrime);
    const TBytes generator{2};
    const std::optional<TBytes> publicA = ModPow(generator, privateA, prime);
    const std::optional<TBytes> publicB = ModPow(generator, privateB, prime);
    Check(publicA && publicB, "DH public keys");
    const std::optional<TBytes> sharedA = ModPow(*publicB, privateA, prime);
    const std::optional<TBytes> sharedB = ModPow(*publicA, privateB, prime);
    Check(sharedA && sharedB && *sharedA == *sharedB, "DH agreement");
    Check(sharedA->size() == prime.size(), "DH secret keeps the prime's length");
    return Json::Object()
        .set("kind", "dh")
        .set("name", name)
        .set("prime", Hex(prime))
        .set("generator", Hex(generator))
        .set("privateA", Hex(privateA))
        .set("privateB", Hex(privateB))
        .set("publicA", Hex(*publicA))
        .set("publicB", Hex(*publicB))
        .set("shared", Hex(*sharedA));
}

SpotifyProtocol::TSha1 Sha1(const TBytes& message) {
    return SpotifyProtocol::Sha1({message});
}

TBytes Pkcs1(std::size_t size, std::uint8_t blockType, const char* digestInfo, const TBytes& message) {
    const TBytes info = Bytes(digestInfo);
    const SpotifyProtocol::TSha1 digest = Sha1(message);
    TBytes encoded{0x00, blockType};
    encoded.insert(encoded.end(), size - 3 - info.size() - digest.size(), 0xff);
    encoded.push_back(0x00);
    encoded.insert(encoded.end(), info.begin(), info.end());
    encoded.insert(encoded.end(), digest.begin(), digest.end());
    return encoded;
}

TBytes Sign(const TBytes& encoded) {
    const std::optional<TBytes> signature =
        ModPow(encoded, Bytes(kSpecRsaPrivateExponent), Bytes(kSpecRsaModulus));
    Check(signature.has_value(), "RSA signing");
    return *signature;
}

Json RsaCase(const std::string& name, const TBytes& modulus, const TBytes& message, const TBytes& signature,
             bool withPrivateExponent) {
    const bool valid = SpotifyProtocol::VerifyRsaSha1(modulus, 65537, Sha1(message), signature);
    std::optional<TBytes> encoded;
    if (signature.size() <= modulus.size()) {
        TBytes padded(modulus.size() - signature.size(), 0);
        padded.insert(padded.end(), signature.begin(), signature.end());
        encoded = ModPow(padded, Bytes("010001"), modulus);
    }
    Json json = Json::Object();
    json.set("kind", "rsa-pkcs1-sha1").set("name", name).set("modulus", Hex(modulus)).set("exponent", 65537);
    if (withPrivateExponent) {
        json.set("privateExponent", kSpecRsaPrivateExponent);
    }
    return json.set("message", Hex(message))
        .set("signature", Hex(signature))
        .set("encoded", encoded ? Json(Hex(*encoded)) : Json())
        .set("valid", valid);
}

}  // namespace

Json ShannonVectors() {
    const TBytes key = Counting(32, 1, 0);
    const TBytes known = Text("spotify shannon kat");
    Json cases = Json::Array();

    {
        ShannonCase reference("the reference's known answer: encrypt under nonce 0", key);
        reference.nonce(0);
        const TBytes output = reference.encrypt(known);
        const TBytes mac = reference.finish();
        Check(Hex(output) == "b90d865fcc7f43ab9648a97e8dec605d95822a", "Shannon known answer");
        Check(Hex(mac) == "bea0ba6c", "Shannon known MAC");
        cases.push(reference.done());
    }
    {
        ShannonCase back("the known answer decrypted: the MAC is over the plaintext", key);
        back.nonce(0);
        Check(back.decrypt(Bytes("b90d865fcc7f43ab9648a97e8dec605d95822a")) == known, "Shannon decrypts back");
        Check(Hex(back.finish()) == "bea0ba6c", "Shannon MAC after decryption");
        cases.push(back.done());
    }
    const TBytes packet = Bytes("1097369f33bec9959eb9656da89b6edc8fc358");
    TBytes split;
    TBytes splitMac;
    {
        ShannonCase received("a received packet: the 3-byte header, then the payload, under one nonce", key);
        received.nonce(1);
        split = received.decrypt(Slice(packet, 0, 3));
        const TBytes rest = received.decrypt(Slice(packet, 3, packet.size() - 3));
        split.insert(split.end(), rest.begin(), rest.end());
        splitMac = received.finish();
        Check(Hex(split) == "ab001030313233343536373839616263646566", "Shannon packet");
        Check(Hex(splitMac) == "8cc90655", "Shannon packet MAC");
        cases.push(received.done());
    }
    {
        ShannonCase whole("the same packet in one call", key);
        whole.nonce(1);
        Check(whole.decrypt(packet) == split, "Shannon packet in one call");
        Check(whole.finish() == splitMac, "Shannon packet MAC in one call");
        cases.push(whole.done());
    }

    const TBytes otherKey = Counting(32, 13, 5);
    const TBytes message = Text("thirteen byte");
    TBytes pieces;
    TBytes piecesMac;
    {
        ShannonCase words("words split over calls of 1, 2, 6 and 4 bytes", otherKey);
        words.nonce(2);
        std::size_t at = 0;
        for (const std::size_t size : {1, 2, 6, 4}) {
            const TBytes part = words.encrypt(Slice(message, at, size));
            pieces.insert(pieces.end(), part.begin(), part.end());
            at += size;
        }
        piecesMac = words.finish();
        cases.push(words.done());
    }
    {
        ShannonCase whole("the same 13 bytes in one call", otherKey);
        whole.nonce(2);
        Check(whole.encrypt(message) == pieces, "Shannon split words");
        Check(whole.finish() == piecesMac, "Shannon split words MAC");
        cases.push(whole.done());
    }
    {
        ShannonCase nonce("the nonce is loaded as four big-endian bytes", otherKey);
        nonce.nonce(0x01020304);
        nonce.encrypt(Text("big-end!"));
        nonce.finish();
        cases.push(nonce.done());
    }
    {
        ShannonCase sequence("packets one after another: each nonce starts afresh", otherKey);
        sequence.nonce(0);
        const TBytes first = sequence.encrypt(Bytes("04000400000001"));
        const TBytes firstMac = sequence.finish();
        sequence.nonce(1);
        sequence.encrypt(Bytes("4a0000"));
        sequence.finish();
        sequence.nonce(0);
        Check(sequence.encrypt(Bytes("04000400000001")) == first, "Shannon nonce restarts");
        Check(sequence.finish() == firstMac, "Shannon nonce restarts the MAC");
        cases.push(sequence.done());
    }
    {
        ShannonCase empty("a MAC with nothing encrypted", otherKey);
        empty.nonce(7);
        empty.finish();
        cases.push(empty.done());
    }

    return Json::Object()
        .set("description",
             "Shannon, the stream cipher and MAC of the access point's packets. Each case makes one cipher from its "
             "key and replays the calls in order.")
        .set("cases", std::move(cases));
}

Json CryptoVectors() {
    Json cases = Json::Array();

    const auto sha1 = [&](const std::string& name, const TBytes& message, const char* expected) {
        const std::string digest = Hex(Sha1(message));
        if (expected) {
            Check(digest == expected, "SHA-1 " + name);
        }
        cases.push(Json::Object().set("kind", "sha1").set("name", name).set("message", Hex(message)).set("digest", digest));
    };
    sha1("FIPS 180-2 A.1: abc", Text("abc"), "a9993e364706816aba3e25717850c26c9cd0d89d");
    sha1("FIPS 180-2 A.2: two blocks", Text("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
         "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
    sha1("an empty message", {}, "da39a3ee5e6b4b0d3255bfef95601890afd80709");

    const auto hmac = [&](const std::string& name, const TBytes& key, const TBytes& message, const char* expected) {
        const std::string digest = Hex(SpotifyProtocol::HmacSha1(key, {message}));
        Check(digest == expected, "HMAC-SHA1 " + name);
        cases.push(Json::Object()
                       .set("kind", "hmac-sha1")
                       .set("name", name)
                       .set("key", Hex(key))
                       .set("message", Hex(message))
                       .set("digest", digest));
    };
    hmac("RFC 2202 case 1", TBytes(20, 0x0b), Text("Hi There"), "b617318655057264e28bc0b6fb378c8ef146be00");
    hmac("RFC 2202 case 2", Text("Jefe"), Text("what do ya want for nothing?"),
         "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79");
    hmac("RFC 2202 case 3", TBytes(20, 0xaa), TBytes(50, 0xdd), "125d7342b9ac11cd91a39af48aa17b4f63f175d3");
    hmac("RFC 2202 case 4", Counting(25, 1, 1), TBytes(50, 0xcd), "4c9007f4026250c6bc8414f9bf50c86c2d7235da");
    hmac("RFC 2202 case 5", TBytes(20, 0x0c), Text("Test With Truncation"),
         "4c1a03424b55e07fe7f27be1d58bb9324a9a5a04");
    hmac("RFC 2202 case 6", TBytes(80, 0xaa), Text("Test Using Larger Than Block-Size Key - Hash Key First"),
         "aa4ae5e15272d00e95705637ce8a3b55ed402112");
    hmac("RFC 2202 case 7", TBytes(80, 0xaa),
         Text("Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data"),
         "e8e99d0f45237d786d6bbaa7965c7808bbff1a91");

    const TBytes spKey = Bytes("2b7e151628aed2a6abf7158809cf4f3c");
    const TBytes spCounter = Bytes("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
    const TBytes spPlain = Bytes(
        "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51"
        "30c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710");
    {
        Json whole = CtrCase("SP 800-38A F.5.1: CTR-AES128.Encrypt", spKey, spCounter, 0, spPlain);
        Check(Hex(Ctr(spKey, spCounter, 0, spPlain)) ==
                  "874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff"
                  "5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee",
              "SP 800-38A F.5.1");
        cases.push(std::move(whole));
    }
    cases.push(CtrCase("F.5.1 from byte 5, inside the first block", spKey, spCounter, 5, Slice(spPlain, 5, 3)));
    cases.push(CtrCase("F.5.1 from byte 13, across a block boundary", spKey, spCounter, 13, Slice(spPlain, 13, 10)));
    cases.push(CtrCase("F.5.1 from byte 16, the second block", spKey, spCounter, 16, Slice(spPlain, 16, 16)));
    cases.push(CtrCase("F.5.1 from byte 37 to the end", spKey, spCounter, 37, Slice(spPlain, 37, 27)));

    const TBytes carryKey(16, 0x03);
    const TBytes carryCounter = Bytes("0000000000000000000000" "01fffffffe");
    cases.push(CtrCase("the counter carries past its low 32-bit word", carryKey, carryCounter, 0, TBytes(64, 0)));
    cases.push(CtrCase("the third block of that run alone", carryKey, carryCounter, 32, TBytes(16, 0)));
    {
        const TBytes carried = Bytes("0000000000000000000000" "0200000000");
        Check(Ctr(carryKey, carried, 0, TBytes(16, 0)) == Ctr(carryKey, carryCounter, 32, TBytes(16, 0)),
              "carried counter");
        cases.push(CtrCase("the same block from the carried counter", carryKey, carried, 0, TBytes(16, 0)));
    }
    cases.push(
        CtrCase("from inside a block, across the low word's wrap", carryKey, carryCounter, 25, Counting(20, 1, 0x40)));
    cases.push(CtrCase("the counter wraps around 2^128", carryKey, Bytes("fffffffffffffffffffffffffffffffe"), 0,
                       TBytes(48, 0)));

    {
        Json small = ModPowCase("4^13 mod 497", {4}, {13}, {0x01, 0xf1});
        Check(Hex(*ModPow({4}, {13}, {0x01, 0xf1})) == "01bd", "ModPow 4^13 mod 0x1f1");
        cases.push(std::move(small));
    }
    cases.push(ModPowCase("a base with leading zero bytes", {0, 0, 4}, {13}, {0x01, 0xf1}));
    cases.push(ModPowCase("the result keeps the modulus's length", {2}, {1}, {0x01, 0x01}));
    cases.push(ModPowCase("exponent zero", {7}, {0}, {0x01, 0xf1}));
    cases.push(ModPowCase("a base not below the modulus is refused", {0x01, 0xf1}, {3}, {0x01, 0xf1}));
    cases.push(ModPowCase("an even modulus is refused", {3}, {3}, {0x01, 0xf0}));

    const TBytes prime = Bytes(kDhPrime);
    const TBytes privateB = Counting(95, 13, 5);
    cases.push(DhCase("Oakley group 1, the private keys of the desktop test", Counting(95, 7, 1), privateB));
    {
        const std::optional<TBytes> publicB = ModPow({2}, privateB, prime);
        std::optional<TBytes> found;
        for (unsigned step = 7; !found && step < 64; step += 2) {
            for (unsigned start = 0; !found && start < 256; ++start) {
                const TBytes candidate = Counting(95, step, start);
                const std::optional<TBytes> shared = ModPow(*publicB, candidate, prime);
                if (shared && (*shared)[0] == 0 && (*shared)[1] != 0) {
                    found = candidate;
                }
            }
        }
        Check(found.has_value(), "a DH secret with a leading zero byte");
        cases.push(DhCase("a shared secret with a leading zero byte", *found, privateB));
    }

    const TBytes desktopModulus = Bytes(kDesktopRsaModulus);
    const TBytes desktopSignature = Bytes(kDesktopRsaSignature);
    cases.push(RsaCase("the desktop test key: abc", desktopModulus, Text("abc"), desktopSignature, false));
    cases.push(RsaCase("the desktop test key: abc's signature on abd", desktopModulus, Text("abd"), desktopSignature,
                       false));

    const TBytes modulus = Bytes(kSpecRsaModulus);
    const TBytes withNull = Sign(Pkcs1(modulus.size(), 1, kDigestInfoWithNull, Text("abc")));
    const TBytes withoutNull = Sign(Pkcs1(modulus.size(), 1, kDigestInfoWithoutNull, Text("abc")));
    cases.push(RsaCase("DigestInfo with the 05 00 NULL parameters", modulus, Text("abc"), withNull, true));
    cases.push(RsaCase("DigestInfo without the NULL parameters", modulus, Text("abc"), withoutNull, true));
    cases.push(RsaCase("abc's signature on abd", modulus, Text("abd"), withNull, true));
    cases.push(RsaCase("block type 02 instead of 01", modulus, Text("abc"),
                       Sign(Pkcs1(modulus.size(), 2, kDigestInfoWithNull, Text("abc"))), true));
    {
        std::optional<std::pair<TBytes, TBytes>> found;
        for (int n = 0; !found && n < 100000; ++n) {
            const TBytes message = Text("leading zero " + std::to_string(n));
            const TBytes signature = Sign(Pkcs1(modulus.size(), 1, kDigestInfoWithNull, message));
            if (signature[0] == 0 && signature[1] != 0) {
                found = {message, Slice(signature, 1, signature.size() - 1)};
            }
        }
        Check(found.has_value(), "a signature with a leading zero byte");
        cases.push(RsaCase("a signature one byte shorter than the modulus: its leading zero left out", modulus,
                           found->first, found->second, true));
    }
    cases.push(RsaCase("a signature not below the modulus", modulus, Text("abc"), modulus, true));
    {
        const TBytes longer = Join({0}, withNull);
        Check(!SpotifyProtocol::VerifyRsaSha1(modulus, 65537, Sha1(Text("abc")), longer), "a longer signature");
        cases.push(RsaCase("a signature longer than the modulus, even by a leading zero byte", modulus, Text("abc"),
                           longer, true));
    }

    return Json::Object()
        .set("description",
             "SHA-1, HMAC-SHA1, AES-128-CTR from any offset, modular power, Diffie-Hellman in Oakley group 1 and "
             "RSA PKCS#1 v1.5 SHA-1 signature checks, as the Spotify protocol uses them.")
        .set("cases", std::move(cases));
}

Json AudioVectors() {
    const TBytes iv = Bytes(kAudioIv);
    const TBytes key(16, 0x07);
    const TBytes plain = Counting(200, 1, 0);
    const SpotifyProtocol::AudioDecryptor decryptor(AesKey(key));
    TBytes encrypted = plain;
    decryptor.apply(encrypted.data(), encrypted.size(), 0);
    Check(encrypted == Ctr(key, iv, 0, plain), "the audio IV");
    Check(SpotifyProtocol::kOggHeaderSize == 167, "the header size");

    Json cases = Json::Array();
    const auto decrypt = [&](const std::string& name, std::uint64_t offset, const TBytes& input) {
        TBytes output = input;
        decryptor.apply(output.data(), output.size(), offset);
        CheckCtrAgainstBlocks(key, iv, offset, input, output);
        cases.push(Json::Object()
                       .set("kind", "decrypt")
                       .set("name", name)
                       .set("key", Hex(key))
                       .set("iv", Hex(iv))
                       .set("offset", offset)
                       .set("encrypted", Hex(input))
                       .set("decrypted", Hex(output)));
        return output;
    };
    const auto range = [&](const std::string& name, std::size_t offset, std::size_t size) {
        Check(decrypt(name, offset, Slice(encrypted, offset, size)) == Slice(plain, offset, size),
              "audio range " + name);
    };
    range("a whole 200-byte file", 0, 200);
    range("3 bytes from byte 5", 5, 3);
    range("77 bytes from byte 21", 21, 77);
    for (std::size_t offset = 160; offset <= 170; ++offset) {
        range("10 bytes from byte " + std::to_string(offset), offset, 10);
    }

    const std::uint64_t lowWordWrap = ((std::uint64_t{1} << 32) - 0x3f630d93) * 16;
    decrypt("a huge offset: the counter's low word wraps", lowWordWrap - 8, TBytes(32, 0));
    decrypt("a huge offset: block 2^32", (std::uint64_t{1} << 36) - 8, TBytes(32, 0));

    cases.push(Json::Object()
                   .set("kind", "header")
                   .set("name", "the 200-byte file: the first 167 decrypted bytes are Spotify's header")
                   .set("key", Hex(key))
                   .set("iv", Hex(iv))
                   .set("encrypted", Hex(encrypted))
                   .set("headerSize", SpotifyProtocol::kOggHeaderSize)
                   .set("audio", Hex(Slice(plain, 167, plain.size() - 167))));

    return Json::Object()
        .set("description",
             "Decrypting a Spotify audio file: AES-128-CTR with the fixed IV 72e067fbddcbcf77ebe8bc643f630d93 from "
             "any byte offset, and the 167-byte header in front of the Ogg Vorbis stream.")
        .set("cases", std::move(cases));
}

}  // namespace Vectors
