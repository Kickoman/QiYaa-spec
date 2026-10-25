// Validates fixtures/spotify/ and expected/spotify/: every fixture has an expected file and every
// expected file a fixture, every endpoint and case is in the case table of fixtures/spotify/README.md,
// case names carry their HTTP status as the spec requires, every file is formatted as the spec
// requires, and a protobuf fixture is a {contentType, base64, fields} wrapper whose fields encode to
// exactly its bytes. Then recomputes every expected file from its fixture with parsers written from
// the two READMEs (a list page's requested offset comes from its case name, `page-<n>`): extended-metadata (tracks with their availability, covers and the file to play,
// albums, artists and their top tracks), the collection, the rootlist and playlists, storage-resolve,
// context-resolve, Pathfinder's home and search, and apresolve.
import { readdirSync, readFileSync, statSync } from "node:fs";
import { join } from "node:path";
import { fileURLToPath } from "node:url";

const fixtureRoot = fileURLToPath(new URL("../fixtures/spotify/", import.meta.url));
const expectedRoot = fileURLToPath(new URL("../expected/spotify/", import.meta.url));
const failures = [];

const protobufEndpoints = {
  "extended-metadata": "application/x-protobuf",
  "collection-paging": "application/vnd.collection-v2.spotify.proto",
  "playlist-user-rootlist": "application/x-protobuf",
  playlist: "application/x-protobuf",
  "storage-resolve-files-audio-interactive": "application/x-protobuf",
};
const jsonEndpoints = ["context-resolve", "context-resolve-autoplay", "radio-apollo-tracks", "pathfinder-home", "pathfinder-searchDesktop", "apresolve"];
const base62Digits = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const madeForYou = "spotify:section:0JQ5DAUnp4wcj0bCb3wh3S";
const coverHost = "https://i.scdn.co/image/";
const listPage = 1000;

const show = (value) => JSON.stringify(value, (_, x) => (typeof x === "bigint" ? `${x}n` : x));
const formatted = (data) => JSON.stringify(data, null, 2) + "\n";
const hex = (data) => Buffer.from(data).toString("hex");
const utf8 = (data) => Buffer.from(data).toString("utf8");

function directories(root) {
  return readdirSync(root).filter((name) => statSync(join(root, name)).isDirectory()).sort();
}

function cases(root, endpoint) {
  return readdirSync(join(root, endpoint)).filter((name) => name.endsWith(".json")).map((name) => name.slice(0, -5)).sort();
}

function varintBytes(value) {
  const out = [];
  for (; value >= 0x80n; value >>= 7n) out.push(Number(value & 0x7fn) | 0x80);
  out.push(Number(value));
  return Buffer.from(out);
}

function encodeFields(fields) {
  return Buffer.concat(
    fields.map((field) => {
      if ("varint" in field) return Buffer.concat([varintBytes(BigInt(field.field) << 3n), varintBytes(BigInt(field.varint))]);
      const payload =
        "string" in field ? Buffer.from(field.string, "utf8")
        : "bytes" in field ? Buffer.from(field.bytes, "hex")
        : encodeFields(field.message);
      return Buffer.concat([varintBytes((BigInt(field.field) << 3n) | 2n), varintBytes(BigInt(payload.length)), payload]);
    }),
  );
}

function fieldsOf(data) {
  const list = [];
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
    if (key === null || key >> 3n === 0n || key >> 3n > 0x1fffffffn) return { list, failed: true };
    const field = Number(key >> 3n);
    const wire = Number(key & 7n);
    if (wire === 0) {
      const value = readVarint();
      if (value === null) return { list, failed: true };
      list.push({ field, value, data: Buffer.alloc(0) });
    } else if (wire === 1 || wire === 5) {
      const width = wire === 1 ? 8 : 4;
      if (data.length - at < width) return { list, failed: true };
      const chunk = data.subarray(at, at + width);
      at += width;
      list.push({ field, value: BigInt(`0x${Buffer.from(chunk).reverse().toString("hex")}`), data: chunk });
    } else if (wire === 2) {
      const length = readVarint();
      if (length === null || length > BigInt(data.length - at)) return { list, failed: true };
      list.push({ field, value: 0n, data: data.subarray(at, at + Number(length)) });
      at += Number(length);
    } else {
      return { list, failed: true };
    }
  }
  return { list, failed: false };
}

const zigzag = (value) => Number((value >> 1n) ^ -(value & 1n));
const int32 = (value) => Number(BigInt.asIntN(32, value));
const uint32 = (value) => Number(BigInt.asUintN(32, value));

function base62(gid) {
  let value = BigInt(`0x${hex(gid) || "0"}`);
  let out = "";
  for (let i = 0; i < 22; i++, value /= 62n) out = base62Digits[Number(value % 62n)] + out;
  return out;
}

function copyId(data, size, fallback) {
  return data.length === size ? Buffer.from(data) : fallback;
}

function gidOf(data) {
  let gid = Buffer.alloc(16);
  for (const f of fieldsOf(data).list) if (f.field === 1) gid = copyId(f.data, 16, gid);
  return gid;
}

function imageOf(data) {
  let id = Buffer.alloc(20);
  let haveId = false;
  let size = 0n;
  let pixels = 0;
  const { list, failed } = fieldsOf(data);
  for (const f of list) {
    if (f.field === 1) {
      haveId = f.data.length === 20;
      if (haveId) id = Buffer.from(f.data);
    } else if (f.field === 2) size = f.value;
    else if (f.field === 3) pixels = int32(BigInt(zigzag(f.value)));
  }
  if (failed || !haveId) return null;
  if (pixels <= 0) pixels = { 1: 64, 2: 640, 3: 1280 }[Number(size)] ?? 300;
  return { id: hex(id), pixels };
}

function artistRefOf(data) {
  const artist = { gid: Buffer.alloc(16), name: "" };
  for (const f of fieldsOf(data).list) {
    if (f.field === 1) artist.gid = copyId(f.data, 16, artist.gid);
    else if (f.field === 2) artist.name = utf8(f.data);
  }
  return artist;
}

function albumRefOf(data) {
  const album = { gid: Buffer.alloc(16), name: "", year: 0, covers: [] };
  for (const f of fieldsOf(data).list) {
    if (f.field === 1) album.gid = copyId(f.data, 16, album.gid);
    else if (f.field === 2) album.name = utf8(f.data);
    else if (f.field === 6) album.year = int32(BigInt(zigzag(fieldsOf(f.data).list.find((x) => x.field === 1)?.value ?? 0n)));
    else if (f.field === 17) {
      for (const image of fieldsOf(f.data).list) {
        if (image.field !== 1) continue;
        const parsed = imageOf(image.data);
        if (parsed) album.covers.push(parsed);
      }
    }
  }
  return album;
}

function audioFileOf(data) {
  let id = null;
  let format = 0;
  const { list, failed } = fieldsOf(data);
  for (const f of list) {
    if (f.field === 1 && f.data.length === 20) id = hex(f.data);
    else if (f.field === 2) format = uint32(f.value);
  }
  return failed || id === null ? null : { id, format };
}

function trackOf(data) {
  const track = { gid: Buffer.alloc(16), name: "", artists: [], album: albumRefOf(Buffer.alloc(0)), durationMs: 0, files: [], alternatives: [] };
  const { list, failed } = fieldsOf(data);
  for (const f of list) {
    if (f.field === 1) track.gid = copyId(f.data, 16, track.gid);
    else if (f.field === 2) track.name = utf8(f.data);
    else if (f.field === 3) track.album = albumRefOf(f.data);
    else if (f.field === 4) track.artists.push(artistRefOf(f.data));
    else if (f.field === 7) track.durationMs = zigzag(f.value);
    else if (f.field === 12) {
      const file = audioFileOf(f.data);
      if (file) track.files.push(file);
    } else if (f.field === 13) {
      const alternative = trackOf(f.data);
      if (alternative) track.alternatives.push(alternative);
    }
  }
  return failed ? null : track;
}

const kbpsOf = (format) => ({ 0: 96, 1: 160, 2: 320 })[format] ?? 0;

function pickOggVorbis(track, preferred) {
  let best = null;
  let bestDistance = 0;
  for (const file of track.files) {
    const kbps = kbpsOf(file.format);
    if (!kbps) continue;
    const distance = Math.abs(kbps - preferred);
    if (!best || distance < bestDistance) {
      best = { trackId: base62(track.gid), fileId: file.id, format: file.format };
      bestDistance = distance;
    }
  }
  if (best) return best;
  for (const alternative of track.alternatives) {
    const file = pickOggVorbis(alternative, preferred);
    if (file) return file;
  }
  return null;
}

function coverUrl(covers, pixels) {
  let best = "";
  let bestPixels = 0;
  for (const { id, pixels: size } of covers) {
    const bigEnough = size >= pixels;
    const bestBigEnough = bestPixels >= pixels;
    if (!best || (bigEnough && (!bestBigEnough || size < bestPixels)) || (!bigEnough && !bestBigEnough && size > bestPixels)) {
      best = id;
      bestPixels = size;
    }
  }
  return best ? coverHost + best : null;
}

const artistsJson = (artists) => artists.map((artist) => ({ id: base62(artist.gid), name: artist.name }));

function trackInfoJson(track) {
  return {
    id: base62(track.gid),
    name: track.name,
    artists: artistsJson(track.artists),
    album: { id: base62(track.album.gid), name: track.album.name, year: track.album.year, covers: track.album.covers },
    durationMs: track.durationMs,
    files: track.files,
    alternatives: track.alternatives.map(trackInfoJson),
  };
}

function trackJson(track) {
  return {
    ...trackInfoJson(track),
    available: pickOggVorbis(track, 320) !== null,
    coverUrls: Object.fromEntries([64, 400, 1000].map((pixels) => [String(pixels), coverUrl(track.album.covers, pixels)])),
    play: Object.fromEntries([96, 160, 320].map((kbps) => [String(kbps), pickOggVorbis(track, kbps)])),
  };
}

function albumOf(data) {
  const album = { gid: Buffer.alloc(16), name: "", artists: [], tracks: [] };
  const { list, failed } = fieldsOf(data);
  for (const f of list) {
    if (f.field === 1) album.gid = copyId(f.data, 16, album.gid);
    else if (f.field === 2) album.name = utf8(f.data);
    else if (f.field === 3) album.artists.push(artistRefOf(f.data));
    else if (f.field === 11) {
      const disc = fieldsOf(f.data);
      for (const entry of disc.list) if (entry.field === 3) album.tracks.push(gidOf(entry.data));
      if (disc.failed) return null;
    }
  }
  return failed ? null : { id: base62(album.gid), name: album.name, artists: artistsJson(album.artists), trackIds: album.tracks.map(base62) };
}

function artistOf(data) {
  const artist = { gid: Buffer.alloc(16), name: "", tops: [] };
  const { list, failed } = fieldsOf(data);
  for (const f of list) {
    if (f.field === 1) artist.gid = copyId(f.data, 16, artist.gid);
    else if (f.field === 2) artist.name = utf8(f.data);
    else if (f.field === 4) {
      const top = { country: "", trackIds: [] };
      const entry = fieldsOf(f.data);
      for (const e of entry.list) {
        if (e.field === 1) top.country = utf8(e.data);
        else if (e.field === 2) top.trackIds.push(base62(gidOf(e.data)));
      }
      if (entry.failed) return null;
      artist.tops.push(top);
    }
  }
  if (failed) return null;
  const topTracksIn = (country) => {
    for (const wanted of [country, "ZZ", ""]) {
      const top = artist.tops.find((t) => t.country === wanted && t.trackIds.length);
      if (top) return top.trackIds;
    }
    return [];
  };
  return {
    id: base62(artist.gid),
    name: artist.name,
    topTracks: artist.tops,
    topTracksIn: Object.fromEntries(["BY", "PL", ""].map((country) => [country, topTracksIn(country)])),
  };
}

function metadataOf(body, name) {
  const key = { tracks: "track", albums: "album", artists: "artist" }[name.split("-")[0]];
  const parse = { track: (d) => (trackOf(d) ? trackJson(trackOf(d)) : null), album: albumOf, artist: artistOf }[key];
  if (!parse) return null;
  const entities = [];
  const top = fieldsOf(body);
  for (const f of top.list) {
    if (f.field !== 2) continue;
    const array = fieldsOf(f.data);
    for (const a of array.list) {
      if (a.field !== 3) continue;
      const entity = { uri: "", status: 0, data: Buffer.alloc(0) };
      const data = fieldsOf(a.data);
      for (const x of data.list) {
        if (x.field === 1) {
          for (const h of fieldsOf(x.data).list) if (h.field === 1) entity.status = int32(h.value);
        } else if (x.field === 2) entity.uri = utf8(x.data);
        else if (x.field === 3) {
          for (const y of fieldsOf(x.data).list) if (y.field === 2) entity.data = y.data;
        }
      }
      if (data.failed) return { unreadable: true };
      entities.push({ uri: entity.uri, status: entity.status, [key]: entity.status === 200 && entity.data.length ? parse(entity.data) : null });
    }
    if (array.failed) return { unreadable: true };
  }
  return top.failed ? { unreadable: true } : { entities };
}

function collectionOf(body) {
  const items = [];
  let nextPageToken = "";
  const { list, failed } = fieldsOf(body);
  for (const f of list) {
    if (f.field === 1) {
      const item = { uri: "", addedAt: 0 };
      let removed = false;
      const entry = fieldsOf(f.data);
      for (const e of entry.list) {
        if (e.field === 1) item.uri = utf8(e.data);
        else if (e.field === 2) item.addedAt = Number(BigInt.asIntN(64, e.value));
        else if (e.field === 3) removed = e.value !== 0n;
      }
      if (!entry.failed && !removed && item.uri) items.push(item);
    } else if (f.field === 2) nextPageToken = utf8(f.data);
  }
  return failed ? { unreadable: true } : { items, nextPageToken, last: !nextPageToken || !items.length };
}

const listName = (data) => {
  const name = fieldsOf(data).list.find((f) => f.field === 1);
  return name ? utf8(name.data) : "";
};

function requestedFromOf(name) {
  const page = /^page-([0-9]+)(-|$)/.exec(name);
  return page ? (Number(page[1]) - 1) * listPage : 0;
}

function listOf(body, rootlist, requestedFrom) {
  const page = { name: "", length: 0, from: 0, truncated: false, items: [] };
  const metas = [];
  const { list, failed } = fieldsOf(body);
  for (const f of list) {
    if (f.field === 2) page.length = int32(f.value);
    else if (f.field === 3) page.name = listName(f.data);
    else if (f.field === 5) {
      const contents = fieldsOf(f.data);
      for (const c of contents.list) {
        if (c.field === 1) page.from = int32(c.value);
        else if (c.field === 2) page.truncated = c.value !== 0n;
        else if (c.field === 3) {
          const item = { uri: "", name: "" };
          for (const e of fieldsOf(c.data).list) if (e.field === 1) item.uri = utf8(e.data);
          page.items.push(item);
        } else if (c.field === 4) {
          const meta = { name: "", status: 0 };
          for (const m of fieldsOf(c.data).list) {
            if (m.field === 2) meta.name = listName(m.data);
            else if (m.field === 9) meta.status = int32(BigInt(zigzag(m.value)));
          }
          metas.push(meta);
        }
      }
      if (contents.failed) return { unreadable: true };
    }
  }
  if (failed) return { unreadable: true };
  if (metas.length === page.items.length) {
    page.items = page.items
      .map((item, i) => ({ uri: item.uri, name: metas[i].name, status: metas[i].status }))
      .filter((item) => item.status === 0 || item.status === 200)
      .map(({ uri, name }) => ({ uri, name }));
  }
  const result = { requestedFrom, ...page, more: requestedFrom + listPage < page.length };
  if (rootlist) result.playlists = page.items.filter((item) => item.uri.startsWith("spotify:playlist:"));
  else result.trackUris = page.items.map((item) => item.uri).filter((uri) => uri.startsWith("spotify:track:"));
  return result;
}

function storageOf(body) {
  let result = 0;
  const urls = [];
  const { list, failed } = fieldsOf(body);
  for (const f of list) {
    if (f.field === 1) result = uint32(f.value);
    else if (f.field === 2) urls.push(utf8(f.data));
  }
  return failed ? { unreadable: true } : { result, urls, usable: result === 0 && urls.length > 0 };
}

const obj = (value) => (value && typeof value === "object" && !Array.isArray(value) ? value : {});
const arr = (value) => (Array.isArray(value) ? value : []);
const str = (value) => (typeof value === "string" ? value : "");
const has = (object, key) => Object.prototype.hasOwnProperty.call(object, key);
const dataOf = (wrapper) => (has(wrapper, "data") ? obj(wrapper.data) : wrapper);

function parseJson(text) {
  try {
    return JSON.parse(text);
  } catch {
    return null;
  }
}

function contextOf(text) {
  const root = parseJson(text);
  if (!root || typeof root !== "object" || Array.isArray(root) || root.loading === true) return { unreadable: true };
  const page = has(root, "pages") ? obj(arr(root.pages)[0]) : root;
  return {
    trackUris: arr(page.tracks).map((track) => str(obj(track).uri)).filter((uri) => uri.startsWith("spotify:track:")),
    next: str(page.next_page_url),
  };
}

function pathfinderOf(status, text, search) {
  const root = obj(parseJson(text));
  const ok = status >= 200 && status < 300;
  const dataIsObject = root.data !== null && typeof root.data === "object" && !Array.isArray(root.data);
  const unparsed = !ok ? { error: { status } } : !dataIsObject ? { unreadable: true } : null;
  if ([400, 405, 412].includes(status) || (ok && !dataIsObject)) return { stale: true, withNoHashLeft: unparsed };
  if (unparsed) return { stale: false, ...unparsed };
  const data = root.data;
  if (search) {
    if (!has(data, "searchV2")) return { stale: false, unreadable: true };
    const found = obj(data.searchV2);
    const trackUris = arr(obj(found.tracksV2).items)
      .map((item) => str(dataOf(obj(obj(item).item)).uri))
      .filter((uri) => uri.startsWith("spotify:track:"));
    const top = arr(obj(found.topResultsV2).itemsV2);
    let kind = "tracks";
    let best = null;
    if (top.length) {
      const item = dataOf(obj(obj(top[0]).item));
      const uri = str(item.uri);
      if (str(item.__typename) === "Artist" && uri.startsWith("spotify:artist:")) {
        kind = "artist";
        best = { uri, name: str(obj(item.profile).name) };
      } else if (str(item.__typename) === "Album" && uri.startsWith("spotify:album:")) {
        kind = "album";
        best = { uri, name: str(item.name) };
      }
    }
    return { stale: false, kind, best, trackUris };
  }
  const home = obj(data.home);
  if (!has(home, "sectionContainer")) return { stale: false, unreadable: true };
  const playlists = [];
  for (const section of arr(obj(obj(home.sectionContainer).sections).items).map(obj)) {
    if (str(section.uri) !== madeForYou) continue;
    for (const card of arr(obj(section.sectionItems).items)) {
      const playlist = obj(obj(obj(card).content).data);
      if (str(playlist.__typename) === "Playlist") playlists.push({ uri: str(playlist.uri), name: str(playlist.name) });
    }
  }
  return { stale: false, playlists };
}

function apresolveOf(text) {
  const root = obj(parseJson(text));
  const strings = (value) => arr(value).map(str).filter((entry) => entry);
  const accessPoints = strings(root.accesspoint);
  const spclients = strings(root.spclient);
  return {
    accessPoints: accessPoints.length ? accessPoints : ["ap.spotify.com:443"],
    spclients: spclients.length ? spclients : ["spclient.wg.spotify.com:443"],
  };
}

function recompute(endpoint, name, status, body) {
  switch (endpoint) {
    case "extended-metadata": return metadataOf(body, name);
    case "collection-paging": return collectionOf(body);
    case "playlist-user-rootlist": return listOf(body, true, requestedFromOf(name));
    case "playlist": return listOf(body, false, requestedFromOf(name));
    case "storage-resolve-files-audio-interactive": return storageOf(body);
    case "context-resolve":
    case "context-resolve-autoplay":
    case "radio-apollo-tracks": return contextOf(utf8(body));
    case "pathfinder-home": return pathfinderOf(status, utf8(body), false);
    case "pathfinder-searchDesktop": return pathfinderOf(status, utf8(body), true);
    case "apresolve": return apresolveOf(utf8(body));
    default: return null;
  }
}

function readFixture(where, endpoint, name, status) {
  const text = readFileSync(join(fixtureRoot, endpoint, `${name}.json`), "utf8");
  if (protobufEndpoints[endpoint]) {
    const wrapper = parseJson(text);
    if (!wrapper || formatted(wrapper) !== text) {
      failures.push(`${where}: a protobuf fixture must be a formatted JSON wrapper`);
      return null;
    }
    if (show(Object.keys(wrapper)) !== '["contentType","base64","fields"]') failures.push(`${where}: the wrapper must hold exactly contentType, base64 and fields`);
    if (wrapper.contentType !== protobufEndpoints[endpoint]) failures.push(`${where}: contentType is ${wrapper.contentType}, not ${protobufEndpoints[endpoint]}`);
    const body = Buffer.from(String(wrapper.base64), "base64");
    if (body.toString("base64") !== wrapper.base64) failures.push(`${where}: base64 is not canonical base64 with padding`);
    try {
      if (!encodeFields(wrapper.fields).equals(body)) failures.push(`${where}: fields do not encode to the base64 body`);
    } catch (error) {
      failures.push(`${where}: unreadable fields (${error.message})`);
    }
    return body;
  }
  if (text === "") {
    if (status < 400) failures.push(`${where}: an empty body is only for an error status`);
  } else {
    const data = parseJson(text);
    if (data === null || typeof data !== "object" || formatted(data) !== text) failures.push(`${where}: not a JSON object formatted with 2-space indentation and a trailing newline`);
  }
  return Buffer.from(text, "utf8");
}

const readme = readFileSync(join(fixtureRoot, "README.md"), "utf8");
const listed = new Map();
for (const row of readme.split("\n").filter((line) => /^\| `[^`]+` \|/.test(line))) {
  const cells = row.split("|").slice(1, -1).map((cell) => cell.trim());
  const endpoint = cells[0].replaceAll("`", "");
  listed.set(endpoint, [...cells[cells.length - 1].matchAll(/`([^`]+)`/g)].map((m) => m[1]).sort());
}
readFileSync(join(expectedRoot, "README.md"), "utf8");

const endpoints = directories(fixtureRoot);
if (show(endpoints) !== show(directories(expectedRoot))) failures.push(`fixtures/spotify and expected/spotify hold different endpoints`);
let count = 0;
for (const endpoint of endpoints) {
  if (!protobufEndpoints[endpoint] && !jsonEndpoints.includes(endpoint)) failures.push(`${endpoint}: not an endpoint this check knows`);
  const fixtureCases = cases(fixtureRoot, endpoint);
  const expectedCases = statSync(join(expectedRoot, endpoint), { throwIfNoEntry: false }) ? cases(expectedRoot, endpoint) : [];
  for (const name of fixtureCases.filter((c) => !expectedCases.includes(c))) failures.push(`${endpoint}/${name}: no expected file`);
  for (const name of expectedCases.filter((c) => !fixtureCases.includes(c))) failures.push(`${endpoint}/${name}: an expected file without a fixture`);
  if (show(listed.get(endpoint) ?? []) !== show(fixtureCases)) failures.push(`${endpoint}: the README lists ${show(listed.get(endpoint) ?? [])}, the folder holds ${show(fixtureCases)}`);
  for (const name of fixtureCases.filter((c) => expectedCases.includes(c))) {
    const where = `${endpoint}/${name}`;
    count++;
    if (!/^([1-5][0-9]{2}-)?[a-z0-9]+(-[a-z0-9]+)*$/.test(name)) failures.push(`${where}: not a lower-case case name with an optional status prefix`);
    const status = /^[1-5][0-9]{2}-/.test(name) ? Number(name.slice(0, 3)) : 200;
    const expectedText = readFileSync(join(expectedRoot, endpoint, `${name}.json`), "utf8");
    const expected = parseJson(expectedText);
    if (!expected || typeof expected !== "object" || Array.isArray(expected) || formatted(expected) !== expectedText) {
      failures.push(`${where}: the expected file is not one JSON object formatted as the spec requires`);
      continue;
    }
    const body = readFixture(where, endpoint, name, status);
    if (body === null) continue;
    try {
      const recomputed = recompute(endpoint, name, status, body);
      if (show(recomputed) !== show(expected)) failures.push(`${where}: expected ${show(expected)}\n  recomputed ${show(recomputed)}`);
    } catch (error) {
      failures.push(`${where}: ${error.message}`);
    }
  }
}
for (const endpoint of listed.keys()) if (!endpoints.includes(endpoint)) failures.push(`${endpoint}: in the README, not in fixtures/spotify`);

if (failures.length) {
  console.error(failures.join("\n"));
  process.exit(1);
}
console.log(`spotify fixtures: ${endpoints.length} endpoints, ${count} cases, every expected file recomputed`);
