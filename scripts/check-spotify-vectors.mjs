// Validates spotify/: the seven vector files are there and README.md describes each, every file is
// one {description, cases} object formatted as the spec requires, case names are unique, and bytes
// are lower-case hex. Then recomputes with node:crypto and BigInt what standard tools can: SHA-1,
// HMAC-SHA1, AES-128-CTR over the whole 128-bit counter (through AES-ECB), the audio decryption with
// Spotify's IV, modular power, Diffie-Hellman, the handshake's keys, RSA PKCS#1 v1.5 SHA-1 checks
// with and without the NULL parameters, base62 ids, hashcash and the protobuf fields that are written.
// Shannon, the access point packets and the request bodies are not recomputed.
import { createCipheriv, createHash, createHmac } from "node:crypto";
import { readdirSync, readFileSync } from "node:fs";
import { join } from "node:path";
import { fileURLToPath } from "node:url";

const root = fileURLToPath(new URL("../spotify/", import.meta.url));
const failures = [];
let recomputed = 0;

const files = ["audio.json", "crypto.json", "handshake.json", "ids.json", "messages.json", "protobuf.json", "shannon.json"];
const byteKeys = new Set([
  "audio", "base", "body", "bytes", "challengeAnswer", "clientPublic", "clientToServer", "command", "context",
  "data", "decrypted", "dhPrime", "dhPrivate", "dhPublic", "digest", "encoded", "encrypted", "exponent", "file",
  "generator", "gid", "input", "iv", "key", "mac", "material", "message", "modulus", "output", "payload", "prefix",
  "prime", "privateA", "privateB", "privateExponent", "publicA", "publicB", "random", "result", "rsaModulus",
  "serverToClient", "shared", "signature", "suffix", "track", "wire",
]);
const textKeys = new Set([
  "base62", "call", "clientId", "clientVersion", "code", "contextUri", "cpuType", "description", "deviceId", "event",
  "hardware", "kind", "name", "os", "pageToken", "recentTrackUris", "release", "set", "state", "string",
  "systemInformation", "systemVersion", "type", "uri", "uris", "username", "value", "varint", "version",
  "versionString", "what", "zigzag",
]);

const oakley1 =
  "ffffffffffffffffc90fdaa22168c234c4c6628b80dc1cd129024e088a67cc74020bbea63b139b22514a08798e3404dd" +
  "ef9519b3cd3a431b302b0a6df25f14374fe1356d6d51c245e485b576625e7ec6f44c42e9a63a3620ffffffffffffffff";
const spotifyIv = "72e067fbddcbcf77ebe8bc643f630d93";
const digestInfo = ["3021300906052b0e03021a05000414", "301f300706052b0e03021a0414"];
const base62Digits = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

const big = (hex) => (hex.length ? BigInt(`0x${hex}`) : 0n);
const bytes = (hex) => Buffer.from(hex, "hex");
const strip = (hex) => hex.replace(/^(00)+/, "");
const sha1 = (...parts) => parts.reduce((hash, part) => hash.update(bytes(part)), createHash("sha1")).digest("hex");
const hmac = (key, ...parts) => parts.reduce((mac, part) => mac.update(bytes(part)), createHmac("sha1", bytes(key))).digest("hex");

function toHex(value, size) {
  const hex = value.toString(16);
  return hex.padStart(Math.max(size * 2, hex.length + (hex.length % 2)), "0");
}

function modPow(base, exponent, modulus) {
  let result = 1n % modulus;
  base %= modulus;
  for (; exponent > 0n; exponent >>= 1n) {
    if (exponent & 1n) result = (result * base) % modulus;
    base = (base * base) % modulus;
  }
  return result;
}

function aesCtr(key, iv, offset, input) {
  const data = bytes(input);
  const skip = offset % 16;
  const blocks = Math.ceil((skip + data.length) / 16);
  const counters = Buffer.alloc(blocks * 16);
  for (let i = 0; i < blocks; i++) {
    const counter = (big(iv) + BigInt(Math.floor(offset / 16)) + BigInt(i)) % (1n << 128n);
    counters.write(toHex(counter, 16), i * 16, "hex");
  }
  const ecb = createCipheriv("aes-128-ecb", bytes(key), null).setAutoPadding(false);
  const stream = Buffer.concat([ecb.update(counters), ecb.final()]);
  return Buffer.from(data.map((byte, i) => byte ^ stream[skip + i])).toString("hex");
}

function rsaCheck(modulus, exponent, digest, signature) {
  const size = modulus.length / 2;
  const s = big(signature);
  if (signature.length / 2 > size || s >= big(modulus)) return { encoded: null, valid: false };
  const encoded = toHex(modPow(s, BigInt(exponent), big(modulus)), size);
  const forms = digestInfo.map((info) => `0001${"ff".repeat(size - 3 - (info.length + digest.length) / 2)}00${info}${digest}`);
  return { encoded, valid: forms.includes(encoded) };
}

function trailingZeros(value) {
  let count = 0;
  for (; count < 64 && (value & 1n) === 0n; count++) value >>= 1n;
  return count;
}

function solveHashcash(context, prefix, length) {
  const target = big(sha1(context).slice(24));
  for (let counter = 0n; counter < 1n << 24n; counter++) {
    const suffix = toHex((target + counter) % (1n << 64n), 8) + toHex(counter, 8);
    if (trailingZeros(big(sha1(prefix, suffix).slice(24))) >= length) return suffix;
  }
  return null;
}

function gidOf(base62) {
  if (base62.length !== 22) return null;
  let value = 0n;
  for (const c of base62) {
    const digit = base62Digits.indexOf(c);
    if (digit < 0) return null;
    value = value * 62n + BigInt(digit);
  }
  return value < 1n << 128n ? toHex(value, 16) : null;
}

function base62Of(gid) {
  let value = big(gid);
  let text = "";
  for (let i = 0; i < 22; i++, value /= 62n) text = base62Digits[Number(value % 62n)] + text;
  return text;
}

function varint(value, out) {
  for (; value >= 0x80n; value >>= 7n) out.push(Number(value & 0x7fn) | 0x80);
  out.push(Number(value));
}

function encodeFields(fields) {
  const out = [];
  for (const field of fields) {
    if ("varint" in field) {
      varint(BigInt(field.field) << 3n, out);
      varint(BigInt(field.varint), out);
      continue;
    }
    const payload =
      "string" in field ? [...Buffer.from(field.string, "utf8")]
      : "bytes" in field ? [...bytes(field.bytes)]
      : encodeFields(field.message);
    varint((BigInt(field.field) << 3n) | 2n, out);
    varint(BigInt(payload.length), out);
    out.push(...payload);
  }
  return out;
}

function expect(where, what, actual, wanted) {
  recomputed++;
  if (actual !== wanted) failures.push(`${where}: ${what} is ${actual}, recomputed ${wanted}`);
}

function checkFormat(name, data, text) {
  if (JSON.stringify(data, null, 2) + "\n" !== text) {
    failures.push(`${name}: not formatted with 2-space indentation and a trailing newline`);
  }
  if (JSON.stringify(Object.keys(data)) !== '["description","cases"]') {
    failures.push(`${name}: the object must hold exactly "description" and "cases"`);
  }
  if (typeof data.description !== "string" || !data.description) failures.push(`${name}: no description`);
  if (!Array.isArray(data.cases) || !data.cases.length) {
    failures.push(`${name}: no cases`);
    return;
  }
  const names = new Set();
  data.cases.forEach((item, index) => {
    if (typeof item.name !== "string" || !item.name) failures.push(`${name}: case ${index} has no name`);
    else if (names.has(item.name)) failures.push(`${name}: "${item.name}" twice`);
    names.add(item.name);
    walk(`${name}: ${item.name ?? index}`, item, null);
  });
}

function walk(where, value, key) {
  if (Array.isArray(value)) {
    value.forEach((item) => walk(where, item, key));
  } else if (value && typeof value === "object") {
    for (const [field, item] of Object.entries(value)) walk(where, item, field);
  } else if (typeof value === "string" && key !== null) {
    if (byteKeys.has(key)) {
      if (!/^([0-9a-f]{2})*$/.test(value)) failures.push(`${where}: ${key} is not lower-case hex bytes`);
    } else if (!textKeys.has(key)) {
      failures.push(`${where}: ${key} is neither a bytes field nor a text field`);
    }
  }
}

function checkCrypto(cases) {
  for (const c of cases) {
    const where = `crypto.json: ${c.name}`;
    if (c.kind === "sha1") {
      expect(where, "digest", c.digest, sha1(c.message));
    } else if (c.kind === "hmac-sha1") {
      expect(where, "digest", c.digest, hmac(c.key, c.message));
    } else if (c.kind === "aes-128-ctr") {
      expect(where, "output", c.output, aesCtr(c.key, c.iv, c.offset, c.input));
    } else if (c.kind === "modpow") {
      const m = big(c.modulus);
      const refused = m === 0n || m % 2n === 0n || big(c.base) >= m;
      expect(where, "result", c.result, refused ? null : toHex(modPow(big(c.base), big(c.exponent), m), c.modulus.length / 2));
    } else if (c.kind === "dh") {
      const p = big(c.prime);
      const size = c.prime.length / 2;
      expect(where, "publicA", c.publicA, toHex(modPow(big(c.generator), big(c.privateA), p), size));
      expect(where, "publicB", c.publicB, toHex(modPow(big(c.generator), big(c.privateB), p), size));
      expect(where, "shared from A", c.shared, toHex(modPow(big(c.publicB), big(c.privateA), p), size));
      expect(where, "shared from B", c.shared, toHex(modPow(big(c.publicA), big(c.privateB), p), size));
    } else if (c.kind === "rsa-pkcs1-sha1") {
      const { encoded, valid } = rsaCheck(c.modulus, c.exponent, sha1(c.message), c.signature);
      expect(where, "encoded", c.encoded, encoded);
      expect(where, "valid", c.valid, valid);
      if (c.privateExponent && encoded) {
        expect(where, "signature from the private exponent", strip(c.signature), strip(toHex(modPow(big(encoded), big(c.privateExponent), big(c.modulus)), 1)));
      }
    } else {
      failures.push(`${where}: unknown kind ${c.kind}`);
    }
  }
}

function checkAudio(cases) {
  for (const c of cases) {
    const where = `audio.json: ${c.name}`;
    expect(where, "iv", c.iv, spotifyIv);
    if (c.kind === "decrypt") {
      expect(where, "decrypted", c.decrypted, aesCtr(c.key, c.iv, c.offset, c.encrypted));
    } else if (c.kind === "header") {
      expect(where, "headerSize", c.headerSize, 167);
      expect(where, "audio", c.audio, aesCtr(c.key, c.iv, 0, c.encrypted).slice(2 * c.headerSize));
    } else {
      failures.push(`${where}: unknown kind ${c.kind}`);
    }
  }
}

function checkHandshake(cases) {
  for (const c of cases) {
    const where = `handshake.json: ${c.name}`;
    expect(where, "dhPrime", c.dhPrime, oakley1);
    const p = big(c.dhPrime);
    const size = c.dhPrime.length / 2;
    expect(where, "server dhPublic", c.server.dhPublic, toHex(modPow(2n, big(c.server.dhPrivate), p), size));
    const gs = c.server.dhPublic;
    expect(where, "the server's signature", rsaCheck(c.server.rsaModulus, c.server.rsaExponent, sha1(gs), c.server.signature).valid, true);
    const trusted = rsaCheck(c.trustedKey.modulus, c.trustedKey.exponent, sha1(gs), c.server.signature).valid;
    expect(where, "random sizes", c.random.map((r) => r.length / 2).join(","), "95,16");

    const hello = c.steps[0]?.outgoing?.[0];
    const response = c.steps[1]?.incoming?.[0];
    if (hello?.what !== "ClientHello" || response?.what !== "APResponseMessage") {
      failures.push(`${where}: the first steps are not the client hello and the access point's response`);
      continue;
    }
    const clientPublic = strip(toHex(modPow(2n, big(c.random[0]), p), size));
    expect(where, "the client hello's prefix and length", hello.wire.slice(0, 12), `0004${toHex(BigInt(hello.wire.length / 2), 4)}`);
    expect(where, "the client hello holds the public key", hello.wire.includes(clientPublic), true);
    expect(where, "the client hello holds the nonce", hello.wire.includes(c.random[1]), true);
    expect(where, "the response's length", response.wire.slice(0, 8), toHex(BigInt(response.wire.length / 2), 4));

    if (c.keys === null) {
      if (trusted && response.wire.includes(c.server.signature)) failures.push(`${where}: a signed response, yet no keys`);
    } else {
      expect(where, "trusted", trusted, true);
      const shared = strip(toHex(modPow(big(gs), big(c.random[0]), p), size));
      expect(where, "clientPublic", c.keys.clientPublic, clientPublic);
      expect(where, "shared", c.keys.shared, shared);
      expect(where, "shared from the server's side", c.keys.shared, strip(toHex(modPow(big(clientPublic), big(c.server.dhPrivate), p), size)));
      const transcript = hello.wire + response.wire;
      const material = [1, 2, 3, 4, 5].map((i) => hmac(shared, transcript, toHex(BigInt(i), 1))).join("");
      expect(where, "material", c.keys.material, material);
      expect(where, "challengeAnswer", c.keys.challengeAnswer, hmac(material.slice(0, 40), transcript));
      expect(where, "clientToServer", c.keys.clientToServer, material.slice(40, 104));
      expect(where, "serverToClient", c.keys.serverToClient, material.slice(104, 168));
      const answer = c.steps[1].outgoing[0];
      expect(where, "the answer holds the HMAC", answer?.what === "ClientResponsePlaintext" && answer.wire.includes(c.keys.challengeAnswer), true);
    }

    const nonces = { incoming: 0, outgoing: 0 };
    for (const step of c.steps) {
      for (const direction of ["incoming", "outgoing"]) {
        for (const message of step[direction] ?? []) {
          if (message.nonce === undefined) continue;
          if (message.nonce !== nonces[direction]++) failures.push(`${where}: ${direction} ${message.what} has nonce ${message.nonce}`);
          if (message.wire.length !== 2 * (3 + message.payload.length / 2 + 4)) failures.push(`${where}: ${message.what}'s wire is not header, payload and MAC`);
        }
      }
    }
  }
}

function checkIds(cases) {
  for (const c of cases) {
    const where = `ids.json: ${c.name}`;
    expect(where, "gid", c.gid, gidOf(c.base62));
    if (c.gid !== null) {
      expect(where, "base62", c.base62, base62Of(c.gid));
      expect(where, "uri", c.uri, `spotify:track:${c.base62}`);
    } else {
      expect(where, "uri", c.uri, null);
    }
  }
}

function checkMessages(cases) {
  for (const c of cases.filter((item) => item.kind === "hashcash")) {
    expect(`messages.json: ${c.name}`, "suffix", c.suffix, solveHashcash(c.context, c.prefix, c.length));
  }
}

function checkProtobuf(cases) {
  for (const c of cases.filter((item) => item.kind === "write")) {
    expect(`protobuf.json: ${c.name}`, "bytes", c.bytes, Buffer.from(encodeFields(c.fields)).toString("hex"));
  }
}

function checkShannon(cases) {
  for (const c of cases) {
    for (const step of c.steps) {
      const bad =
        step.call === "nonce" ? !Number.isInteger(step.value)
        : step.call === "encrypt" || step.call === "decrypt" ? step.input.length !== step.output.length
        : step.call === "finish" ? step.mac.length !== 8
        : true;
      if (bad) failures.push(`shannon.json: ${c.name}: a malformed ${step.call} step`);
    }
  }
}

const present = readdirSync(root).filter((name) => name.endsWith(".json")).sort();
if (JSON.stringify(present) !== JSON.stringify(files)) failures.push(`spotify/: holds ${present.join(", ")}, not ${files.join(", ")}`);
const readme = readFileSync(join(root, "README.md"), "utf8");
const checks = {
  "audio.json": checkAudio,
  "crypto.json": checkCrypto,
  "handshake.json": checkHandshake,
  "ids.json": checkIds,
  "messages.json": checkMessages,
  "protobuf.json": checkProtobuf,
  "shannon.json": checkShannon,
};
let caseCount = 0;
for (const name of files.filter((file) => present.includes(file))) {
  if (!readme.includes(`\`${name}\``)) failures.push(`${name}: not described in spotify/README.md`);
  const text = readFileSync(join(root, name), "utf8");
  const data = JSON.parse(text);
  checkFormat(name, data, text);
  if (Array.isArray(data.cases)) {
    caseCount += data.cases.length;
    checks[name](data.cases);
  }
}

if (failures.length) {
  console.error(failures.join("\n"));
  process.exit(1);
}
console.log(`spotify: ${files.length} files, ${caseCount} cases, ${recomputed} values recomputed, all as expected`);
