// Validates spotify/: the seven vector files are there and README.md describes each, every file is
// one {description, cases} object formatted as the spec requires, case names are unique, bytes are
// lower-case hex, and every kind and call is one the README describes. Then recomputes every result
// from its inputs with node:crypto, BigInt and ports written from spotify/README.md: SHA-1,
// HMAC-SHA1, AES-128-CTR over the whole 128-bit counter (through AES-ECB), the audio decryption,
// modular power, Diffie-Hellman, RSA PKCS#1 v1.5 SHA-1 checks in both DigestInfo forms, Shannon,
// base62 ids, protobuf writing, reading and rejecting, hashcash, the request bodies, and each access
// point session: a client replays its steps whole and byte by byte, and the fake access point's side
// is rebuilt from `server` and `keys`.
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
const spotifyKeySha1 = "e03b91ec2ac5191c0f7c20e953941aa306e98dad";
const spotifyIv = "72e067fbddcbcf77ebe8bc643f630d93";
const digestInfo = ["3021300906052b0e03021a05000414", "301f300706052b0e03021a0414"];
const base62Digits = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const commandNames = {
  0x04: "Ping", 0x0c: "RequestKey", 0x0d: "AesKey", 0x0e: "AesKeyError", 0x1b: "CountryCode", 0x49: "Pong",
  0x4a: "PongAck", 0x50: "ProductInfo", 0x76: "LicenseVersion", 0xab: "Login", 0xac: "APWelcome", 0xad: "AuthFailure",
};

const big = (hex) => (hex.length ? BigInt(`0x${hex}`) : 0n);
const bytes = (hex) => Buffer.from(hex, "hex");
const text = (value) => Buffer.from(value, "utf8");
const cat = (...parts) => Buffer.concat(parts);
const empty = Buffer.alloc(0);
const strip = (hex) => hex.replace(/^(00)+/, "");
const sha1 = (...parts) => parts.reduce((hash, part) => hash.update(bytes(part)), createHash("sha1")).digest("hex");
const hmac = (key, ...parts) => parts.reduce((mac, part) => mac.update(bytes(part)), createHmac("sha1", bytes(key))).digest("hex");
const show = (value) => JSON.stringify(value, (_, x) => (typeof x === "bigint" ? `${x}n` : x));
const same = (a, b) => show(a) === show(b);

function toHex(value, size) {
  const hex = value.toString(16);
  return hex.padStart(Math.max(size * 2, hex.length + (hex.length % 2)), "0");
}

function u32(value) {
  const out = Buffer.alloc(4);
  out.writeUInt32BE(value);
  return out;
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

const rotl = (w, x) => ((w << x) | (w >>> (32 - x))) >>> 0;
const sbox1 = (w) => {
  w = (w ^ (rotl(w, 5) | rotl(w, 7))) >>> 0;
  return (w ^ (rotl(w, 19) | rotl(w, 22))) >>> 0;
};
const sbox2 = (w) => {
  w = (w ^ (rotl(w, 7) | rotl(w, 22))) >>> 0;
  return (w ^ (rotl(w, 5) | rotl(w, 19))) >>> 0;
};
const initKonst = 0x6996c53a;

class Shannon {
  constructor(key) {
    this.r = new Uint32Array(16);
    this.r[0] = this.r[1] = 1;
    for (let i = 2; i < 16; i++) this.r[i] = this.r[i - 1] + this.r[i - 2];
    this.konst = initKonst;
    this.pending = 0;
    this.loadKey(key);
    this.konst = this.r[0];
    this.initial = this.r.slice();
  }

  cycle() {
    const r = this.r;
    let t = sbox1((r[12] ^ r[13] ^ this.konst) >>> 0) ^ rotl(r[0], 1);
    r.copyWithin(0, 1);
    r[15] = t;
    t = sbox2((r[2] ^ r[15]) >>> 0);
    r[0] ^= t;
    this.stream = (t ^ r[8] ^ r[12]) >>> 0;
  }

  loadKey(key) {
    for (let offset = 0; offset < key.length; offset += 4) {
      const word = Buffer.alloc(4);
      key.copy(word, 0, offset, Math.min(offset + 4, key.length));
      this.r[13] ^= word.readUInt32LE(0);
      this.cycle();
    }
    this.r[13] ^= key.length;
    this.cycle();
    this.crc = this.r.slice();
    for (let i = 0; i < 16; i++) this.cycle();
    for (let i = 0; i < 16; i++) this.r[i] ^= this.crc[i];
  }

  nonce(value) {
    this.r = this.initial.slice();
    this.konst = initKonst;
    this.loadKey(u32(value));
    this.konst = this.r[0];
    this.pending = 0;
  }

  mac(word) {
    const crc = this.crc;
    const t = crc[0] ^ crc[2] ^ crc[15] ^ word;
    crc.copyWithin(0, 1);
    crc[15] = t;
    this.r[13] ^= word;
  }

  process(input, encrypt) {
    const data = Buffer.from(input);
    let i = 0;
    const partial = () => {
      const shift = 32 - this.pending;
      if (encrypt) this.macWord = (this.macWord ^ (data[i] << shift)) >>> 0;
      data[i] ^= this.stream >>> shift;
      if (!encrypt) this.macWord = (this.macWord ^ (data[i] << shift)) >>> 0;
      i++;
      this.pending -= 8;
    };
    if (this.pending) {
      while (this.pending && i < data.length) partial();
      if (this.pending) return data;
      this.mac(this.macWord);
    }
    for (; data.length - i >= 4; i += 4) {
      this.cycle();
      let word = data.readUInt32LE(i);
      if (encrypt) this.mac(word);
      word = (word ^ this.stream) >>> 0;
      if (!encrypt) this.mac(word);
      data.writeUInt32LE(word, i);
    }
    if (i < data.length) {
      this.cycle();
      this.macWord = 0;
      this.pending = 32;
      while (i < data.length) partial();
    }
    return data;
  }

  encrypt(data) {
    return this.process(data, true);
  }

  decrypt(data) {
    return this.process(data, false);
  }

  finish(size) {
    if (this.pending) this.mac(this.macWord);
    this.cycle();
    this.r[13] ^= initKonst ^ (this.pending << 3);
    this.pending = 0;
    for (let i = 0; i < 16; i++) this.r[i] ^= this.crc[i];
    for (let i = 0; i < 16; i++) this.cycle();
    const out = Buffer.alloc(size);
    for (let offset = 0; offset < size; offset += 4) {
      this.cycle();
      for (let i = 0; i < 4 && offset + i < size; i++) out[offset + i] = this.stream >>> (8 * i);
    }
    return out;
  }
}

function varint(value) {
  const out = [];
  for (; value >= 0x80n; value >>= 7n) out.push(Number(value & 0x7fn) | 0x80);
  out.push(Number(value));
  return Buffer.from(out);
}

const pv = (field, value) => cat(varint(BigInt(field) << 3n), varint(BigInt(value)));
const pl = (field, data) => cat(varint((BigInt(field) << 3n) | 2n), varint(BigInt(data.length)), data);

function encodeFields(fields) {
  return cat(
    ...fields.map((field) =>
      "varint" in field ? pv(field.field, field.varint)
      : "string" in field ? pl(field.field, text(field.string))
      : "bytes" in field ? pl(field.field, bytes(field.bytes))
      : pl(field.field, encodeFields(field.message))),
  );
}

function readFields(data) {
  const fields = [];
  let at = 0;
  const readVarint = () => {
    let value = 0n;
    for (let shift = 0n; shift < 64n; shift += 7n) {
      if (at >= data.length) return null;
      const byte = data[at++];
      value |= BigInt(byte & 0x7f) << shift;
      if (!(byte & 0x80)) return value & ((1n << 64n) - 1n);
    }
    return null;
  };
  while (at < data.length) {
    const key = readVarint();
    if (key === null || key >> 3n === 0n || key >> 3n > 0x1fffffffn) return { fields, failed: true };
    const field = Number(key >> 3n);
    const wire = Number(key & 7n);
    if (wire === 0) {
      const value = readVarint();
      if (value === null) return { fields, failed: true };
      fields.push({ field, wire, varint: String(value), zigzag: String((value >> 1n) ^ -(value & 1n)) });
    } else if (wire === 1 || wire === 5) {
      const width = wire === 1 ? 8 : 4;
      if (data.length - at < width) return { fields, failed: true };
      const chunk = data.subarray(at, at + width);
      at += width;
      fields.push({ field, wire, bytes: chunk.toString("hex"), value: String(big(Buffer.from(chunk).reverse().toString("hex"))) });
    } else if (wire === 2) {
      const length = readVarint();
      if (length === null || length > BigInt(data.length - at)) return { fields, failed: true };
      fields.push({ field, wire, bytes: data.subarray(at, at + Number(length)).toString("hex") });
      at += Number(length);
    } else {
      return { fields, failed: true };
    }
  }
  return { fields, failed: false };
}

function parse(data) {
  if (!Buffer.isBuffer(data)) return null;
  const { fields, failed } = readFields(data);
  if (failed) return null;
  const map = new Map();
  for (const f of fields) map.set(f.field, f.wire === 0 ? BigInt(f.varint) : bytes(f.bytes));
  return map;
}

const nested = (message, ...path) => path.reduce((at, field) => (at ? parse(at.get(field)) : null), message);

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
  let out = "";
  for (let i = 0; i < 22; i++, value /= 62n) out = base62Digits[Number(value % 62n)] + out;
  return out;
}

function expect(where, what, actual, wanted) {
  recomputed++;
  if (!same(actual, wanted)) failures.push(`${where}: ${what} is ${show(actual)}, recomputed ${show(wanted)}`);
}

function unknown(where, what, value) {
  failures.push(`${where}: unknown ${what} ${JSON.stringify(value)}`);
}

function checkFormat(name, data, source) {
  if (JSON.stringify(data, null, 2) + "\n" !== source) {
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

function checkShannon(cases) {
  for (const c of cases) {
    const cipher = new Shannon(bytes(c.key));
    c.steps.forEach((step, index) => {
      const where = `shannon.json: ${c.name}: step ${index}`;
      if (step.call === "nonce") cipher.nonce(step.value);
      else if (step.call === "encrypt") expect(where, "output", step.output, cipher.encrypt(bytes(step.input)).toString("hex"));
      else if (step.call === "decrypt") expect(where, "output", step.output, cipher.decrypt(bytes(step.input)).toString("hex"));
      else if (step.call === "finish") expect(where, "mac", step.mac, cipher.finish(step.mac.length / 2).toString("hex"));
      else unknown(where, "call", step.call);
    });
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
      expect(where, "prime", c.prime, oakley1);
      expect(where, "publicA", c.publicA, toHex(modPow(big(c.generator), big(c.privateA), p), size));
      expect(where, "publicB", c.publicB, toHex(modPow(big(c.generator), big(c.privateB), p), size));
      expect(where, "shared from A", c.shared, toHex(modPow(big(c.publicB), big(c.privateA), p), size));
      expect(where, "shared from B", c.shared, toHex(modPow(big(c.publicA), big(c.privateB), p), size));
    } else if (c.kind === "rsa-pkcs1-sha1") {
      const { encoded, valid } = rsaCheck(c.modulus, c.exponent, sha1(c.message), c.signature);
      expect(where, "encoded", c.encoded, encoded);
      expect(where, "valid", c.valid, valid);
      if (c.privateExponent !== undefined) {
        const n = big(c.modulus);
        expect(where, "privateExponent inverts exponent", modPow(modPow(2n, BigInt(c.exponent), n), big(c.privateExponent), n), 2n);
      }
    } else {
      unknown(where, "kind", c.kind);
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
      unknown(where, "kind", c.kind);
    }
  }
}

class Client {
  constructor(c) {
    this.c = c;
    this.stage = "idle";
    this.calls = 0;
    this.incoming = empty;
    this.pending = null;
    this.sendNonce = 0;
    this.receiveNonce = 0;
    this.keepAlive = "ping";
    this.deadlineAt = null;
    this.nextSequence = 0;
    this.outgoing = [];
    this.received = [];
    this.events = [];
  }

  take(list) {
    const taken = this[list];
    this[list] = [];
    return taken;
  }

  deadline() {
    return this.stage === "ready" ? this.deadlineAt : null;
  }

  fail() {
    this.stage = "failed";
    this.events.push({ event: "failure" });
  }

  start() {
    const random = (size) => bytes(this.c.random[this.calls++] ?? "").subarray(0, size);
    this.private = random(95);
    const nonce = random(16);
    const { identity } = this.c;
    const p = big(this.c.dhPrime);
    const clientPublic = bytes(strip(toHex(modPow(2n, big(this.private.toString("hex")), p), this.c.dhPrime.length / 2)));
    const buildInfo = cat(pv(10, 0), pv(20, 0), pv(30, identity.platform), pv(40, identity.protocolVersion));
    const hello = cat(
      pl(10, buildInfo),
      pv(30, 0),
      pl(50, pl(10, cat(pl(10, clientPublic), pv(20, 1)))),
      pl(60, nonce),
      pl(70, Buffer.from([0x1e])),
    );
    this.transcript = cat(Buffer.from([0, 4]), u32(6 + hello.length), hello);
    this.outgoing.push({ what: "ClientHello", wire: this.transcript.toString("hex") });
    this.stage = "handshake";
  }

  receive(data, nowMs) {
    if (this.stage === "idle" || this.stage === "failed") return;
    this.incoming = cat(this.incoming, data);
    if (this.stage === "handshake") this.readHandshake();
    if (this.stage === "login" || this.stage === "ready") this.readPackets(nowMs);
  }

  readHandshake() {
    if (this.incoming.length < 4) return;
    const size = this.incoming.readUInt32BE(0);
    if (size < 4 || size > 64 * 1024) {
      this.fail();
      return;
    }
    if (this.incoming.length < size) return;
    const wire = this.incoming.subarray(0, size);
    this.incoming = this.incoming.subarray(size);
    this.transcript = cat(this.transcript, wire);
    this.received.push({ what: "APResponseMessage", wire: wire.toString("hex") });
    this.answerChallenge(wire.subarray(4));
  }

  loginFailed(data) {
    const failed = parse(data);
    if (!failed) return null;
    return { event: "loginFailed", code: Number(failed.get(10) ?? 0n), description: (failed.get(40) ?? empty).toString("utf8") };
  }

  answerChallenge(reply) {
    const response = parse(reply);
    if (!response) {
      this.fail();
      return;
    }
    const refused = response.has(30) ? this.loginFailed(response.get(30)) : null;
    if (refused) {
      this.events.push(refused);
      this.stage = "failed";
      return;
    }
    const dh = nested(response, 10, 10, 10);
    const gs = dh?.get(10) ?? empty;
    const signature = dh?.get(30) ?? empty;
    const p = big(this.c.dhPrime);
    const { trustedKey } = this.c;
    if (!gs.length || !signature.length || big(gs.toString("hex")) >= p) {
      this.fail();
      return;
    }
    if (!rsaCheck(trustedKey.modulus, trustedKey.exponent, sha1(gs.toString("hex")), signature.toString("hex")).valid) {
      this.fail();
      return;
    }
    const shared = strip(toHex(modPow(big(gs.toString("hex")), big(this.private.toString("hex")), p), this.c.dhPrime.length / 2));
    const transcript = this.transcript.toString("hex");
    const material = bytes([1, 2, 3, 4, 5].map((i) => hmac(shared, transcript, toHex(BigInt(i), 1))).join(""));
    const answer = bytes(hmac(material.subarray(0, 20).toString("hex"), transcript));
    this.sendCipher = new Shannon(material.subarray(20, 52));
    this.receiveCipher = new Shannon(material.subarray(52, 84));
    const plaintext = cat(pl(10, pl(10, pl(10, answer))), pl(20, empty), pl(30, empty));
    this.outgoing.push({ what: "ClientResponsePlaintext", wire: cat(u32(4 + plaintext.length), plaintext).toString("hex") });
    this.stage = "login";
    const { credentials, identity } = this.c;
    const login = cat(credentials.username ? pl(10, text(credentials.username)) : empty, pv(20, credentials.type), pl(30, bytes(credentials.data)));
    const system = cat(pv(10, identity.cpuFamily), pv(60, identity.os), pl(90, text(identity.systemInformation)), pl(100, text(identity.deviceId)));
    this.send(0xab, cat(pl(10, login), pl(50, system), pl(70, text(identity.versionString))));
  }

  send(command, payload) {
    const nonce = this.sendNonce++;
    this.sendCipher.nonce(nonce);
    const encrypted = this.sendCipher.encrypt(cat(Buffer.from([command, payload.length >> 8, payload.length & 0xff]), payload));
    const wire = cat(encrypted, this.sendCipher.finish(4));
    this.outgoing.push(packetJson(command, nonce, payload, wire));
  }

  readPackets(nowMs) {
    while (this.stage !== "failed") {
      if (!this.pending) {
        if (this.incoming.length < 3) return;
        const header = this.incoming.subarray(0, 3);
        this.incoming = this.incoming.subarray(3);
        this.receiveCipher.nonce(this.receiveNonce);
        const plain = this.receiveCipher.decrypt(header);
        this.pending = { nonce: this.receiveNonce++, command: plain[0], size: plain.readUInt16BE(1), header };
        continue;
      }
      const { nonce, command, size, header } = this.pending;
      if (this.incoming.length < size + 4) return;
      const body = this.incoming.subarray(0, size);
      const payload = this.receiveCipher.decrypt(body);
      const mac = this.receiveCipher.finish(4);
      if (!mac.equals(this.incoming.subarray(size, size + 4))) {
        this.fail();
        return;
      }
      this.received.push(packetJson(command, nonce, payload, cat(header, body, mac)));
      this.incoming = this.incoming.subarray(size + 4);
      this.pending = null;
      this.handle(command, payload, nowMs);
    }
  }

  handle(command, payload, nowMs) {
    if (command === 0xac) {
      const welcome = parse(payload);
      if (!welcome) {
        this.fail();
        return;
      }
      this.stage = "ready";
      this.keepAlive = "ping";
      this.deadlineAt = nowMs + 20000;
      this.events.push({
        event: "loggedIn",
        username: (welcome.get(10) ?? empty).toString("utf8"),
        type: Number(welcome.get(30) ?? 1n),
        data: (welcome.get(40) ?? empty).toString("hex"),
      });
    } else if (command === 0xad) {
      this.events.push(this.loginFailed(payload) ?? { event: "loginFailed", code: 0, description: "" });
      this.stage = "failed";
    } else if (command === 0x04) {
      this.keepAlive = "pong";
      this.deadlineAt = nowMs + 60000;
    } else if (command === 0x4a) {
      this.keepAlive = "ping";
      this.deadlineAt = nowMs + 80000;
    } else if (command === 0x50) {
      const xml = payload.toString("utf8");
      const start = xml.indexOf("<type>");
      const end = start < 0 ? -1 : xml.indexOf("</type>", start + 6);
      this.events.push({ event: "product", type: end < 0 ? "" : xml.slice(start + 6, end) });
    } else if (command === 0x1b) {
      this.events.push({ event: "country", code: payload.toString("utf8") });
    } else if (command === 0x0d && payload.length === 20) {
      this.events.push({ event: "audioKey", sequence: payload.readUInt32BE(0), key: payload.subarray(4).toString("hex") });
    } else if (command === 0x0e && payload.length >= 6) {
      this.events.push({ event: "audioKeyRefused", sequence: payload.readUInt32BE(0), code: payload.readUInt16BE(4) });
    }
  }

  advance(nowMs) {
    if (this.stage !== "ready" || this.deadlineAt === null || nowMs < this.deadlineAt) return;
    if (this.keepAlive !== "pong") {
      this.fail();
      return;
    }
    this.send(0x49, Buffer.alloc(4));
    this.keepAlive = "pongAck";
    this.deadlineAt = nowMs + 20000;
  }

  requestAudioKey(track, file) {
    if (this.stage !== "ready") return null;
    const sequence = this.nextSequence++;
    this.send(0x0c, cat(file, track, u32(sequence), Buffer.from([0, 0])));
    return sequence;
  }
}

function packetJson(command, nonce, payload, wire) {
  return {
    what: commandNames[command] ?? "unknown",
    nonce,
    command: toHex(BigInt(command), 1),
    payload: payload.toString("hex"),
    wire: wire.toString("hex"),
  };
}

function replay(c, where, byteByByte) {
  const client = new Client(c);
  c.steps.forEach((step, index) => {
    const at = `${where}: step ${index} (${step.call})${byteByByte ? ", byte by byte" : ""}`;
    if (step.call === "start") {
      client.start();
    } else if (step.call === "receive") {
      const data = bytes(step.incoming.map((message) => message.wire).join(""));
      if (byteByByte) for (let i = 0; i < data.length; i++) client.receive(data.subarray(i, i + 1), step.nowMs);
      else client.receive(data, step.nowMs);
      expect(at, "incoming", step.incoming, client.take("received"));
    } else if (step.call === "advance") {
      client.advance(step.nowMs);
    } else if (step.call === "requestAudioKey") {
      expect(at, "returns", step.returns, client.requestAudioKey(bytes(step.track), bytes(step.file)));
    } else {
      unknown(at, "call", step.call);
      return;
    }
    expect(at, "outgoing", step.outgoing, client.take("outgoing"));
    expect(at, "events", step.events, client.take("events"));
    expect(at, "deadline", step.deadline, client.deadline());
    expect(at, "loggedIn", step.loggedIn, client.stage === "ready");
    expect(at, "failed", step.failed, client.stage === "failed");
  });
}

function checkHandshake(cases) {
  for (const c of cases) {
    const where = `handshake.json: ${c.name}`;
    expect(where, "dhPrime", c.dhPrime, oakley1);
    const p = big(c.dhPrime);
    const size = c.dhPrime.length / 2;
    const { server } = c;
    expect(where, "random sizes", c.random.map((r) => r.length / 2), [95, 16]);
    expect(where, "server dhPrivate", server.dhPrivate, Buffer.from(Array.from({ length: 95 }, (_, i) => (i * 7 + 3) & 0xff)).toString("hex"));
    expect(where, "server dhPublic", server.dhPublic, toHex(modPow(2n, big(server.dhPrivate), p), size));
    expect(where, "the server's signature", rsaCheck(server.rsaModulus, server.rsaExponent, sha1(server.dhPublic), server.signature).valid, true);
    expect(where, "trustedKey is the fake access point's or Spotify's",
      c.trustedKey.modulus === server.rsaModulus || sha1(c.trustedKey.modulus) === spotifyKeySha1, true);
    expect(where, "trustedKey exponent", c.trustedKey.exponent, 65537);

    if (c.steps[0]?.call !== "start" || c.steps[1]?.incoming?.[0]?.what !== "APResponseMessage") {
      failures.push(`${where}: the first steps are not the start and the access point's response`);
      continue;
    }
    replay(c, where, false);
    replay(c, where, true);

    const refused = c.steps[1].events.find((event) => event.event === "loginFailed");
    const body = refused
      ? pl(30, cat(pv(10, refused.code), refused.description ? pl(40, text(refused.description)) : empty))
      : pl(10, pl(10, pl(10, cat(pl(10, bytes(server.dhPublic)), pv(20, 1), pl(30, bytes(server.signature))))));
    const response = c.steps[1].incoming[0].wire;
    expect(where, "the response built from server", response, cat(u32(4 + body.length), body).toString("hex"));

    if (c.keys === null) continue;
    const hello = c.steps[0].outgoing[0].wire;
    const clientPublic = strip(toHex(modPow(2n, big(c.random[0]), p), size));
    const shared = strip(toHex(modPow(big(server.dhPublic), big(c.random[0]), p), size));
    expect(where, "clientPublic", c.keys.clientPublic, clientPublic);
    expect(where, "shared", c.keys.shared, shared);
    expect(where, "shared from the server's side", c.keys.shared, strip(toHex(modPow(big(clientPublic), big(server.dhPrivate), p), size)));
    const transcript = hello + response;
    const material = [1, 2, 3, 4, 5].map((i) => hmac(shared, transcript, toHex(BigInt(i), 1))).join("");
    expect(where, "material", c.keys.material, material);
    expect(where, "challengeAnswer", c.keys.challengeAnswer, hmac(material.slice(0, 40), transcript));
    expect(where, "clientToServer", c.keys.clientToServer, material.slice(40, 104));
    expect(where, "serverToClient", c.keys.serverToClient, material.slice(104, 168));

    const serverSend = new Shannon(bytes(c.keys.serverToClient));
    const serverReceive = new Shannon(bytes(c.keys.clientToServer));
    for (const step of c.steps) {
      for (const message of step.incoming ?? []) {
        if (message.nonce === undefined) continue;
        const payload = bytes(message.payload);
        serverSend.nonce(message.nonce);
        const encrypted = serverSend.encrypt(cat(bytes(message.command), Buffer.from([payload.length >> 8, payload.length & 0xff]), payload));
        expect(`${where}: ${message.what} ${message.nonce}`, "wire sent by the server", message.wire, cat(encrypted, serverSend.finish(4)).toString("hex"));
      }
      for (const message of step.outgoing) {
        if (message.nonce === undefined) continue;
        const wire = bytes(message.wire);
        serverReceive.nonce(message.nonce);
        const plain = serverReceive.decrypt(wire.subarray(0, wire.length - 4));
        const mac = serverReceive.finish(4).toString("hex");
        expect(`${where}: ${message.what} ${message.nonce}`, "packet read by the server",
          [message.command, message.payload, wire.subarray(wire.length - 4).toString("hex")],
          [plain.subarray(0, 1).toString("hex"), plain.subarray(3).toString("hex"), mac]);
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

function platformData(platform) {
  if (platform.os === "linux") {
    return pl(5, cat(pl(1, text("Linux")), pl(2, text(platform.release)), pl(3, text(platform.version)), pl(4, text(platform.hardware))));
  }
  if (platform.os === "windows") {
    return pl(4, cat(pv(1, platform.osVersion), pv(3, platform.osBuild), pv(4, 2), pv(6, 9), pv(7, platform.imageFileMachine),
      pv(8, platform.peMachine), pv(10, 1)));
  }
  if (platform.os === "macos") {
    return pl(3, cat(pl(1, text(platform.systemVersion)), pl(2, text("iMac21,1")), pl(3, text(platform.cpuType))));
  }
  return null;
}

function messageBody(c) {
  if (c.kind === "login5") {
    let solution = empty;
    if (c.hashcash !== null) {
      const seconds = Math.floor(c.hashcash.tookNanos / 1e9);
      const nanos = c.hashcash.tookNanos % 1e9;
      const duration = cat(seconds ? pv(1, seconds) : empty, nanos ? pv(2, nanos) : empty);
      solution = pl(3, pl(1, pl(1, cat(pl(1, bytes(c.hashcash.suffix)), pl(2, duration)))));
    }
    return cat(
      pl(1, cat(pl(1, text(c.clientId)), pl(2, text(c.deviceId)))),
      c.context ? pl(2, bytes(c.context)) : empty,
      solution,
      pl(100, cat(pl(1, text(c.username)), pl(2, bytes(c.data)))),
    );
  }
  if (c.kind === "clientToken") {
    const platform = platformData(c.platform);
    if (!platform) return null;
    return cat(pv(1, 1), pl(2, cat(pl(1, text(c.clientVersion)), pl(2, text(c.clientId)), pl(3, cat(pl(1, platform), pl(2, text(c.deviceId)))))));
  }
  if (c.kind === "clientTokenAnswer") {
    return cat(pv(1, 2), pl(3, cat(pl(1, text(c.state)), pl(2, cat(pv(1, 3), pl(4, pl(1, text(c.suffix.toUpperCase()))))))));
  }
  if (c.kind === "metadata") {
    return cat(...c.uris.map((uri) => pl(2, cat(pl(1, text(uri)), pl(2, pv(1, c.extensionKind))))));
  }
  if (c.kind === "autoplay") {
    return cat(pl(1, text(c.contextUri)), ...c.recentTrackUris.map((uri) => pl(2, text(uri))));
  }
  if (c.kind === "collection") {
    return cat(pl(1, text(c.username)), pl(2, text(c.set)), c.pageToken ? pl(3, text(c.pageToken)) : empty, pv(4, c.limit));
  }
  return null;
}

function checkMessages(cases) {
  for (const c of cases) {
    const where = `messages.json: ${c.name}`;
    if (c.kind === "hashcash") {
      expect(where, "suffix", c.suffix, solveHashcash(c.context, c.prefix, c.length));
      continue;
    }
    const body = messageBody(c);
    if (body === null) unknown(where, "kind or platform", c.kind);
    else expect(where, "body", c.body, body.toString("hex"));
  }
}

function checkProtobuf(cases) {
  for (const c of cases) {
    const where = `protobuf.json: ${c.name}`;
    if (c.kind === "write") {
      expect(where, "bytes", c.bytes, encodeFields(c.fields).toString("hex"));
    } else if (c.kind === "read") {
      expect(where, "fields", { fields: c.fields, failed: false }, readFields(bytes(c.bytes)));
    } else if (c.kind === "reject") {
      expect(where, "rejected", { fields: c.fieldsBefore, failed: true }, readFields(bytes(c.bytes)));
    } else {
      unknown(where, "kind", c.kind);
    }
  }
}

const present = readdirSync(root).filter((name) => name.endsWith(".json")).sort();
if (!same(present, files)) failures.push(`spotify/: holds ${present.join(", ")}, not ${files.join(", ")}`);
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
  const source = readFileSync(join(root, name), "utf8");
  const data = JSON.parse(source);
  checkFormat(name, data, source);
  if (Array.isArray(data.cases)) {
    caseCount += data.cases.length;
    try {
      checks[name](data.cases);
    } catch (error) {
      failures.push(`${name}: ${error.message}`);
    }
  }
}

if (failures.length) {
  console.error(failures.join("\n"));
  process.exit(1);
}
console.log(`spotify: ${files.length} files, ${caseCount} cases, ${recomputed} values recomputed, all as expected`);
