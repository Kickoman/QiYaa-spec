#include "fields.h"
#include "vectors.h"

#include "sources/spotify/library.cpp"
#include "spotify/client/session.cpp"

#include "sources/spotify/tracks.h"
#include "spotify/protocol/catalog.h"
#include "spotify/protocol/ids.h"
#include "spotify/protocol/metadata.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace SpotifyClient {

void Web::spclient(const WebRequest&, TReply) {
    std::abort();
}

void Web::pathfinder(const QByteArray&, TReply) {
    std::abort();
}

void Web::report(WebProblem, int) {
    std::abort();
}

}  // namespace SpotifyClient

namespace Vectors {

namespace {

namespace fs = std::filesystem;
using SpotifyProtocol::TGid;

constexpr char kProtobufType[] = "application/x-protobuf";
constexpr int kCoverSizes[] = {64, 400, 1000};
constexpr int kBitrates[] = {96, 160, 320};
const char* const kCountries[] = {"BY", "PL", ""};

std::string Base62(const TBytes& gid) {
    TGid id{};
    Check(gid.size() == id.size(), "gid size");
    std::copy(gid.begin(), gid.end(), id.begin());
    return SpotifyProtocol::Base62FromGid(id);
}

std::string Base62(const TGid& gid) {
    return SpotifyProtocol::Base62FromGid(gid);
}

TBytes Gid(std::uint8_t mark) {
    return TBytes(16, mark);
}

TBytes FileId(std::uint8_t mark) {
    return TBytes(20, mark);
}

std::string Uri(const char* kind, std::uint8_t mark) {
    return std::string("spotify:") + kind + ":" + Base62(Gid(mark));
}

std::string Std(const QString& text) {
    return text.toStdString();
}

Json Strings(const std::vector<std::string>& texts) {
    Json json = Json::Array();
    for (const std::string& text : texts) {
        json.push(text);
    }
    return json;
}

Json Strings(const QStringList& texts) {
    Json json = Json::Array();
    for (const QString& text : texts) {
        json.push(Std(text));
    }
    return json;
}

class Output {
public:
    explicit Output(std::string specRoot)
        : root(std::move(specRoot)) {
        for (const char* side : {"fixtures", "expected"}) {
            const fs::path base = fs::path(root) / side / "spotify";
            if (!fs::exists(base)) {
                continue;
            }
            for (const fs::directory_entry& endpoint : fs::directory_iterator(base)) {
                if (endpoint.is_directory()) {
                    fs::remove_all(endpoint.path());
                }
            }
        }
    }

    void write(const std::string& endpoint, const std::string& name, const std::string& fixture, const Json& expected) {
        put(fs::path(root) / "fixtures" / "spotify" / endpoint / (name + ".json"), fixture);
        put(fs::path(root) / "expected" / "spotify" / endpoint / (name + ".json"), expected.dump());
        ++count;
    }

    int count = 0;

private:
    static void put(const fs::path& path, const std::string& text) {
        fs::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << text;
        out.close();
        Check(static_cast<bool>(out), "could not write " + path.string());
    }

    std::string root;
};

std::string Wrapped(const char* contentType, const std::vector<Field>& fields, TBytes* body) {
    *body = Write(fields).data();
    return Json::Object()
        .set("contentType", contentType)
        .set("base64", Base64(*body))
        .set("fields", FieldsJson(fields))
        .dump();
}

std::vector<Field> Image(std::uint8_t mark, std::uint64_t size, int width) {
    std::vector<Field> image{B(1, FileId(mark)), V(2, size)};
    if (width > 0) {
        image.push_back(V(3, static_cast<std::uint64_t>(width) * 2));
        image.push_back(V(4, static_cast<std::uint64_t>(width) * 2));
    }
    return image;
}

std::vector<Field> ArtistRef(std::uint8_t mark, const std::string& name) {
    return {B(1, Gid(mark)), S(2, name)};
}

struct AlbumSpec {
    std::uint8_t mark = 0xab;
    std::string name = "Album";
    int year = 1991;
    std::vector<std::vector<Field>> covers;
};

AlbumSpec DesktopAlbum() {
    AlbumSpec album;
    album.covers = {Image(0x64, 1, 64), Image(0x30, 0, 300), Image(0x40, 2, 0)};
    return album;
}

std::vector<Field> AlbumRef(const AlbumSpec& album) {
    std::vector<Field> fields{B(1, Gid(album.mark)), S(2, album.name), M(3, ArtistRef(0xa0, "Band"))};
    if (album.year != 0) {
        fields.push_back(M(6, {V(1, static_cast<std::uint64_t>(album.year) * 2), V(2, 6 * 2), V(3, 17 * 2)}));
    }
    if (!album.covers.empty()) {
        std::vector<Field> group;
        for (const std::vector<Field>& image : album.covers) {
            group.push_back(M(1, image));
        }
        fields.push_back(M(17, group));
    }
    return fields;
}

struct TrackSpec {
    std::uint8_t mark = 0;
    std::string name;
    std::vector<std::pair<std::uint8_t, std::uint64_t>> files;
    std::vector<std::vector<Field>> alternatives;
    std::vector<std::vector<Field>> artists{ArtistRef(0xa0, "Band")};
    AlbumSpec album = DesktopAlbum();
    std::vector<Field> extra;
    std::uint64_t durationMs = 200000;
};

TrackSpec NewTrack(std::uint8_t mark, std::string name) {
    TrackSpec track;
    track.mark = mark;
    track.name = std::move(name);
    return track;
}

std::vector<Field> FilesOf(const std::vector<std::pair<std::uint8_t, std::uint64_t>>& files) {
    std::vector<Field> fields;
    for (const auto& [mark, format] : files) {
        fields.push_back(M(12, {B(1, FileId(mark)), V(2, format)}));
    }
    return fields;
}

std::vector<Field> TrackFields(const TrackSpec& track) {
    std::vector<Field> fields{B(1, Gid(track.mark)), S(2, track.name), M(3, AlbumRef(track.album))};
    for (const std::vector<Field>& artist : track.artists) {
        fields.push_back(M(4, artist));
    }
    fields.push_back(V(7, track.durationMs * 2));
    fields.insert(fields.end(), track.extra.begin(), track.extra.end());
    for (Field& file : FilesOf(track.files)) {
        fields.push_back(std::move(file));
    }
    for (const std::vector<Field>& alternative : track.alternatives) {
        fields.push_back(M(13, alternative));
    }
    return fields;
}

std::vector<Field> Alternative(std::uint8_t mark, std::uint64_t format) {
    return {B(1, Gid(mark)), M(12, {B(1, FileId(mark)), V(2, format)})};
}

Field Entity(const std::string& uri, int status, const char* type, std::optional<std::vector<Field>> payload) {
    std::vector<Field> fields{M(1, {V(1, static_cast<std::uint64_t>(status))}), S(2, uri)};
    if (payload) {
        fields.push_back(M(3, {S(1, std::string("type.googleapis.com/spotify.metadata.") + type), M(2, *payload)}));
    }
    return M(3, fields);
}

std::vector<Field> MetadataReply(SpotifyProtocol::ExtensionKind kind, std::vector<Field> entities) {
    std::vector<Field> array{V(2, static_cast<std::uint64_t>(kind))};
    for (Field& entity : entities) {
        array.push_back(std::move(entity));
    }
    return {M(2, array)};
}

Json ArtistsJson(const std::vector<SpotifyProtocol::ArtistRef>& artists) {
    Json json = Json::Array();
    for (const SpotifyProtocol::ArtistRef& artist : artists) {
        json.push(Json::Object().set("id", Base62(artist.gid)).set("name", artist.name));
    }
    return json;
}

Json IdsJson(const std::vector<TGid>& gids) {
    Json json = Json::Array();
    for (const TGid& gid : gids) {
        json.push(Base62(gid));
    }
    return json;
}

Json TrackInfoJson(const SpotifyProtocol::TrackInfo& track) {
    Json covers = Json::Array();
    for (const SpotifyProtocol::Image& image : track.album.covers) {
        covers.push(Json::Object().set("id", Hex(image.id)).set("pixels", image.pixels));
    }
    Json files = Json::Array();
    for (const SpotifyProtocol::AudioFileRef& file : track.files) {
        files.push(Json::Object().set("id", Hex(file.id)).set("format", file.format));
    }
    Json alternatives = Json::Array();
    for (const SpotifyProtocol::TrackInfo& alternative : track.alternatives) {
        alternatives.push(TrackInfoJson(alternative));
    }
    return Json::Object()
        .set("id", Base62(track.gid))
        .set("name", track.name)
        .set("artists", ArtistsJson(track.artists))
        .set("album",
             Json::Object()
                 .set("id", Base62(track.album.gid))
                 .set("name", track.album.name)
                 .set("year", track.album.year)
                 .set("covers", std::move(covers)))
        .set("durationMs", track.durationMs)
        .set("files", std::move(files))
        .set("alternatives", std::move(alternatives));
}

Json TrackJson(const SpotifyProtocol::TrackInfo& info) {
    const Core::Track track = SpotifySource::TrackOf(info);
    Json json = TrackInfoJson(info);
    json.set("available", track.available);
    Json urls = Json::Object();
    for (const int pixels : kCoverSizes) {
        const QString url = SpotifySource::CoverUrlOf(track, pixels).toString();
        urls.set(std::to_string(pixels), url.isEmpty() ? Json() : Json(Std(url)));
    }
    json.set("coverUrls", std::move(urls));
    Json play = Json::Object();
    for (const int kbps : kBitrates) {
        const std::optional<SpotifyProtocol::PlayableFile> file = SpotifyProtocol::PickOggVorbis(info, kbps);
        play.set(std::to_string(kbps),
                 file ? Json(Json::Object()
                                 .set("trackId", Base62(file->track))
                                 .set("fileId", Hex(file->file.id))
                                 .set("format", file->file.format))
                      : Json());
    }
    return json.set("play", std::move(play));
}

Json AlbumJson(const SpotifyProtocol::AlbumInfo& album) {
    return Json::Object()
        .set("id", Base62(album.gid))
        .set("name", album.name)
        .set("artists", ArtistsJson(album.artists))
        .set("trackIds", IdsJson(album.tracks));
}

Json ArtistJson(const SpotifyProtocol::ArtistInfo& artist) {
    Json tops = Json::Array();
    for (const SpotifyProtocol::TopTracks& top : artist.topTracks) {
        tops.push(Json::Object().set("country", top.country).set("trackIds", IdsJson(top.tracks)));
    }
    Json in = Json::Object();
    for (const char* country : kCountries) {
        in.set(country, IdsJson(SpotifyProtocol::TopTracksIn(artist, country)));
    }
    return Json::Object()
        .set("id", Base62(artist.gid))
        .set("name", artist.name)
        .set("topTracks", std::move(tops))
        .set("topTracksIn", std::move(in));
}

Json MetadataExpected(const TBytes& body, const char* key) {
    const auto entities = SpotifyProtocol::ParseMetadataReply(body);
    if (!entities) {
        return Json::Object().set("unreadable", true);
    }
    Json list = Json::Array();
    for (const SpotifyProtocol::EntityData& entity : *entities) {
        Json parsed;
        if (entity.status == 200 && !entity.data.empty()) {
            if (std::string(key) == "track") {
                const auto track = SpotifyProtocol::ParseTrack(entity.data);
                Check(track.has_value(), "track payload");
                parsed = TrackJson(*track);
            } else if (std::string(key) == "album") {
                const auto album = SpotifyProtocol::ParseAlbum(entity.data);
                Check(album.has_value(), "album payload");
                parsed = AlbumJson(*album);
            } else {
                const auto artist = SpotifyProtocol::ParseArtist(entity.data);
                Check(artist.has_value(), "artist payload");
                parsed = ArtistJson(*artist);
            }
        }
        list.push(Json::Object().set("uri", entity.uri).set("status", entity.status).set(key, std::move(parsed)));
    }
    return Json::Object().set("entities", std::move(list));
}

void ExtendedMetadata(Output& out) {
    using SpotifyProtocol::AudioFormat;
    const auto format = [](AudioFormat value) { return static_cast<std::uint64_t>(value); };

    TrackSpec plays = NewTrack(1, "Plays");
    plays.files = {{0x01, format(AudioFormat::OggVorbis96)},  {0x02, format(AudioFormat::OggVorbis160)},
                   {0x03, format(AudioFormat::OggVorbis320)}, {0x04, format(AudioFormat::Mp3_320)},
                   {0x05, format(AudioFormat::Flac)},         {0x06, format(AudioFormat::Aac24)}};
    plays.artists.push_back(ArtistRef(0xa1, "Guest"));
    plays.extra = {V(5, 1), V(6, 1), V(8, 54)};
    TrackSpec relinked = NewTrack(2, "Relinked");
    relinked.alternatives = {Alternative(0x22, format(AudioFormat::OggVorbis320))};
    TrackSpec gone = NewTrack(3, "Gone");
    TrackSpec lossless = NewTrack(4, "Lossless only");
    lossless.files = {{0x41, format(AudioFormat::Flac)},
                      {0x42, format(AudioFormat::Flac24Bit)},
                      {0x43, format(AudioFormat::Aac24)}};
    lossless.album.covers = {Image(0x45, 3, 0), Image(0x44, 2, 0)};
    TrackSpec oddCovers = NewTrack(9, "Width before size class");
    oddCovers.files = {{0x92, format(AudioFormat::OggVorbis160)}};
    oddCovers.album.covers = {
        Image(0x95, 1, 500),
        {B(1, FileId(0x96)), V(2, 2), V(3, 0)},
        {B(1, FileId(0x97)), V(2, 3), V(3, 1)},
        Image(0x98, 0, 120),
    };
    TrackSpec middle = NewTrack(5, "Middle");
    middle.files = {{0x51, format(AudioFormat::Flac)},
                    {0x52, format(AudioFormat::OggVorbis160)},
                    {0x53, format(AudioFormat::Mp3_320)}};
    middle.album.year = 0;
    middle.album.covers = {};
    TrackSpec restricted = NewTrack(6, "Restricted elsewhere");
    restricted.files = {{0x62, format(AudioFormat::OggVorbis160)}};
    restricted.extra = {M(11, {S(3, "PL")})};

    TBytes body;
    const std::string fixture = Wrapped(
        kProtobufType,
        MetadataReply(SpotifyProtocol::ExtensionKind::TrackV4,
                      {
                          Entity(Uri("track", 9), 200, "Track", TrackFields(oddCovers)),
                          Entity(Uri("track", 8), 200, "Track", std::nullopt),
                          Entity(Uri("track", 7), 404, "Track", std::nullopt),
                          Entity(Uri("track", 6), 200, "Track", TrackFields(restricted)),
                          Entity(Uri("track", 5), 200, "Track", TrackFields(middle)),
                          Entity(Uri("track", 4), 200, "Track", TrackFields(lossless)),
                          Entity(Uri("track", 3), 200, "Track", TrackFields(gone)),
                          Entity(Uri("track", 2), 200, "Track", TrackFields(relinked)),
                          Entity(Uri("track", 1), 200, "Track", TrackFields(plays)),
                      }),
        &body);
    out.write("extended-metadata", "tracks", fixture, MetadataExpected(body, "track"));
    const std::string broken = Wrapped(
        kProtobufType,
        {M(2, {V(2, static_cast<std::uint64_t>(SpotifyProtocol::ExtensionKind::TrackV4)),
               Entity(Uri("track", 1), 200, "Track", TrackFields(plays)), B(3, Bytes("0a0568656c"))})},
        &body);
    out.write("extended-metadata", "tracks-broken-entity", broken, MetadataExpected(body, "track"));

    const auto disc = [](int number, std::initializer_list<std::uint8_t> tracks) {
        std::vector<Field> fields{V(1, static_cast<std::uint64_t>(number) * 2)};
        for (const std::uint8_t mark : tracks) {
            fields.push_back(M(3, {B(1, Gid(mark))}));
        }
        return M(11, fields);
    };
    const std::vector<Field> doubleAlbum{
        B(1, Gid(0x71)), S(2, "Double"), M(3, ArtistRef(0x81, "Band")), V(4, 1), disc(1, {3, 4}), disc(2, {5}),
    };
    const std::vector<Field> single{B(1, Gid(0x72)), S(2, "Single"), M(3, ArtistRef(0x82, "Worldwide")), disc(1, {9})};
    const std::vector<Field> outOfOrder{
        B(1, Gid(0x74)), S(2, "Discs out of order"), M(3, ArtistRef(0x81, "Band")), disc(2, {0x0d}), disc(1, {0x0c}),
    };
    const std::string albums = Wrapped(
        kProtobufType,
        MetadataReply(SpotifyProtocol::ExtensionKind::AlbumV4,
                      {
                          Entity(Uri("album", 0x74), 200, "Album", outOfOrder),
                          Entity(Uri("album", 0x73), 404, "Album", std::nullopt),
                          Entity(Uri("album", 0x72), 200, "Album", single),
                          Entity(Uri("album", 0x71), 200, "Album", doubleAlbum),
                      }),
        &body);
    out.write("extended-metadata", "albums", albums, MetadataExpected(body, "album"));

    const auto top = [](const std::string& country, std::initializer_list<std::uint8_t> tracks) {
        std::vector<Field> fields{S(1, country)};
        for (const std::uint8_t mark : tracks) {
            fields.push_back(M(2, {B(1, Gid(mark))}));
        }
        return M(4, fields);
    };
    const std::vector<Field> band{B(1, Gid(0x81)), S(2, "Band"), top("ZZ", {6}), top("BY", {7})};
    const std::vector<Field> worldwide{
        B(1, Gid(0x82)), S(2, "Worldwide"), top("BY", {}),
        top("ZZ", {0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c}),
    };
    const std::vector<Field> noCountry{B(1, Gid(0x83)), S(2, "No country"), top("", {0xa9})};
    const std::string artists = Wrapped(
        kProtobufType,
        MetadataReply(SpotifyProtocol::ExtensionKind::ArtistV4,
                      {
                          Entity(Uri("artist", 0x83), 200, "Artist", noCountry),
                          Entity(Uri("artist", 0x82), 200, "Artist", worldwide),
                          Entity(Uri("artist", 0x81), 200, "Artist", band),
                      }),
        &body);
    out.write("extended-metadata", "artists", artists, MetadataExpected(body, "artist"));
}

Json CollectionExpected(const TBytes& body) {
    const auto page = SpotifyProtocol::ParseCollectionPage(body);
    if (!page) {
        return Json::Object().set("unreadable", true);
    }
    Json items = Json::Array();
    for (const SpotifyProtocol::CollectionItem& item : page->items) {
        items.push(Json::Object().set("uri", item.uri).set("addedAt", item.addedAt));
    }
    return Json::Object()
        .set("items", std::move(items))
        .set("nextPageToken", page->nextPageToken)
        .set("last", page->nextPageToken.empty() || page->items.empty());
}

void CollectionPaging(Output& out) {
    const auto item = [](const std::string& uri, std::uint64_t addedAt, bool removed = false) {
        std::vector<Field> fields{S(1, uri), V(2, addedAt)};
        if (removed) {
            fields.push_back(V(3, 1));
        }
        return M(1, fields);
    };
    const std::vector<std::pair<std::string, std::vector<Field>>> pages{
        {"liked-page-1",
         {item(Uri("track", 1), 1729000600), item(Uri("album", 0x71), 1729000500),
          item(Uri("track", 0x0b), 1729000450, true), item(Uri("track", 2), 1729000400),
          item(Uri("track", 3), 1729000300), S(2, "synthetic-page-2"), S(3, "synthetic-sync-token")}},
        {"liked-page-2", {item(Uri("track", 4), 1729000200), S(3, "synthetic-sync-token")}},
        {"artists", {item(Uri("artist", 0x81), 1729000100), item(Uri("artist", 0x82), 1729000000)}},
        {"empty", {S(2, "synthetic-page-3")}},
    };
    for (const auto& [name, fields] : pages) {
        TBytes body;
        const std::string fixture = Wrapped(SpotifyProtocol::kCollectionContentType, fields, &body);
        out.write("collection-paging", name, fixture, CollectionExpected(body));
    }
}

Field ListContents(int from, bool truncated, const std::vector<std::vector<Field>>& items,
                   const std::vector<std::vector<Field>>& metas) {
    std::vector<Field> fields{V(1, static_cast<std::uint64_t>(from)), V(2, truncated ? 1 : 0)};
    for (const std::vector<Field>& item : items) {
        fields.push_back(M(3, item));
    }
    for (const std::vector<Field>& meta : metas) {
        fields.push_back(M(4, meta));
    }
    return M(5, fields);
}

Json ListExpected(const TBytes& body, bool rootlist, int requestedFrom) {
    const auto page = SpotifyProtocol::ParseListPage(body);
    if (!page) {
        return Json::Object().set("unreadable", true);
    }
    Json items = Json::Array();
    for (const SpotifyProtocol::ListItem& item : page->items) {
        items.push(Json::Object().set("uri", item.uri).set("name", item.name));
    }
    Json json = Json::Object()
                    .set("requestedFrom", requestedFrom)
                    .set("name", page->name)
                    .set("length", page->length)
                    .set("from", page->from)
                    .set("truncated", page->truncated)
                    .set("items", std::move(items))
                    .set("more", requestedFrom + SpotifySource::Library::kListPage < page->length);
    if (rootlist) {
        Json playlists = Json::Array();
        for (const SpotifyProtocol::ListItem& item : SpotifyProtocol::PlaylistsOf(*page)) {
            playlists.push(Json::Object().set("uri", item.uri).set("name", item.name));
        }
        json.set("playlists", std::move(playlists));
    } else {
        QStringList uris;
        for (const SpotifyProtocol::ListItem& item : page->items) {
            uris << QString::fromStdString(item.uri);
        }
        json.set("trackUris", Strings(SpotifySource::WithPrefix(uris, SpotifySource::kTrackPrefix)));
    }
    return json;
}

void Playlists(Output& out) {
    const auto meta = [](const std::string& name, int status) {
        std::vector<Field> fields{B(1, Counting(12, 3, 1)), M(2, {S(1, name)}), V(3, 3)};
        if (status != 0) {
            fields.push_back(V(9, static_cast<std::uint64_t>(status) * 2));
        }
        return fields;
    };
    const std::vector<std::pair<std::string, std::string>> rootlist{
        {"spotify:start-group:2d5f9a0c3b1e4a7f:Road trips", ""},
        {Uri("playlist", 0x51), "Road"},
        {Uri("playlist", 0x53), "Shared, then hidden"},
        {"spotify:end-group:2d5f9a0c3b1e4a7f", ""},
        {Uri("playlist", 0x52), "Home"},
        {Uri("playlist", 0x54), "Status 200"},
    };
    std::vector<std::vector<Field>> items;
    std::vector<std::vector<Field>> metas;
    for (const auto& [uri, name] : rootlist) {
        items.push_back({S(1, uri)});
        const int status = uri == Uri("playlist", 0x53) ? 403 : uri == Uri("playlist", 0x54) ? 200 : 0;
        metas.push_back(meta(name, status));
    }
    TBytes body;
    std::string fixture = Wrapped(kProtobufType,
                                  {B(1, Counting(12, 7, 2)), V(2, rootlist.size()), ListContents(0, false, items, metas)},
                                  &body);
    out.write("playlist-user-rootlist", "folders", fixture, ListExpected(body, true, 0));
    fixture = Wrapped(kProtobufType, {V(2, rootlist.size()), ListContents(0, false, items, {})}, &body);
    out.write("playlist-user-rootlist", "without-attributes", fixture, ListExpected(body, true, 0));

    const auto entry = [](const std::string& uri) {
        return std::vector<Field>{S(1, uri), M(2, {S(1, "owner"), V(2, 1729000000000)})};
    };
    const std::vector<Field> name{S(1, "Road")};
    fixture = Wrapped(kProtobufType,
                      {B(1, Counting(12, 5, 3)), V(2, 1003), M(3, name),
                       ListContents(0, true,
                                    {entry(Uri("track", 1)), entry("spotify:episode:" + Base62(Gid(0xe1))),
                                     entry("spotify:local:Some+Band:Some+Album:Own+recording:241"),
                                     entry(Uri("track", 2))},
                                    {})},
                      &body);
    out.write("playlist", "page-1", fixture, ListExpected(body, false, 0));
    const std::vector<std::vector<Field>> second{entry(Uri("track", 3)), entry(Uri("track", 4)), entry(Uri("track", 5))};
    fixture = Wrapped(kProtobufType, {B(1, Counting(12, 5, 3)), V(2, 1003), M(3, name), ListContents(1000, false, second, {})},
                      &body);
    out.write("playlist", "page-2", fixture, ListExpected(body, false, 1000));
    std::vector<Field> withoutPosition;
    for (const std::vector<Field>& item : second) {
        withoutPosition.push_back(M(3, item));
    }
    fixture = Wrapped(kProtobufType, {B(1, Counting(12, 5, 3)), V(2, 1003), M(3, name), M(5, withoutPosition)}, &body);
    out.write("playlist", "page-2-without-position", fixture, ListExpected(body, false, 1000));
}

void StorageResolve(Output& out) {
    const std::string file = Hex(FileId(0x03));
    const std::vector<std::pair<std::string, std::vector<Field>>> cases{
        {"cdn",
         {V(1, 0), S(2, "https://audio-fa.scdn.co/audio/" + file + "?1729003600_synthetic-token"),
          S(2, "https://audio-ak.spotifycdn.com/audio/" + file + "?__token__=synthetic-token"), B(4, FileId(0x03)),
          V(5, 86400)}},
        {"storage", {V(1, 1), B(4, FileId(0x03))}},
        {"restricted", {V(1, 3), B(4, FileId(0x03))}},
        {"cdn-without-urls", {V(1, 0), B(4, FileId(0x03))}},
    };
    for (const auto& [name, fields] : cases) {
        TBytes body;
        const std::string fixture = Wrapped(kProtobufType, fields, &body);
        const auto storage = SpotifyProtocol::ParseStorageResolve(body);
        if (!storage) {
            out.write("storage-resolve-files-audio-interactive", name, fixture, Json::Object().set("unreadable", true));
            continue;
        }
        const bool usable = storage->result == SpotifyProtocol::StorageResult::Cdn && !storage->urls.empty();
        out.write("storage-resolve-files-audio-interactive", name, fixture,
                  Json::Object()
                      .set("result", static_cast<unsigned>(storage->result))
                      .set("urls", Strings(storage->urls))
                      .set("usable", usable));
    }
}

Json ContextTracks(const std::vector<std::string>& uris) {
    Json tracks = Json::Array();
    for (const std::string& uri : uris) {
        tracks.push(Json::Object()
                        .set("uri", uri)
                        .set("uid", "synthetic-uid-" + uri.substr(uri.size() - 4))
                        .set("metadata", Json::Object().set("decision_id", "synthetic-decision")));
    }
    return tracks;
}

Json ContextExpected(const std::string& body) {
    const std::optional<SpotifySource::ContextUris> context =
        SpotifySource::ContextOf(QByteArray::fromStdString(body));
    if (!context) {
        return Json::Object().set("unreadable", true);
    }
    return Json::Object().set("trackUris", Strings(context->uris)).set("next", Std(context->next));
}

void Contexts(Output& out) {
    const std::string stationUri = "spotify:station:track:" + Base62(Gid(1));
    const std::string station =
        Json::Object()
            .set("uri", stationUri)
            .set("url", "context://" + stationUri)
            .set("metadata", Json::Object().set("context_description", "Plays Radio"))
            .set("pages",
                 Json::Array()
                     .push(Json::Object()
                               .set("tracks", ContextTracks({Uri("track", 21), "spotify:episode:" + Base62(Gid(0xe2)),
                                                             Uri("track", 22)}))
                               .set("next_page_url",
                                    "hm://radio-apollo/v5/tracks/" + stationUri + "?salt=2&prev_tracks=21,22"))
                     .push(Json::Object().set("tracks", ContextTracks({Uri("track", 23)}))))
            .dump();
    out.write("context-resolve", "station", station, ContextExpected(station));

    const std::string autoplayUri = "spotify:station:user:owner:collection";
    const std::string autoplay =
        Json::Object()
            .set("uri", autoplayUri)
            .set("pages", Json::Array().push(
                              Json::Object()
                                  .set("tracks", ContextTracks({Uri("track", 11), Uri("track", 12)}))
                                  .set("next_page_url",
                                       "hm://radio-apollo/v5/tracks/spotify:station:x?salt=1&prev_tracks=a,b")))
            .dump();
    out.write("context-resolve-autoplay", "liked-songs", autoplay, ContextExpected(autoplay));
    const std::string loading = Json::Object().set("loading", true).dump();
    out.write("context-resolve-autoplay", "loading", loading, ContextExpected(loading));

    const std::string next = Json::Object().set("tracks", ContextTracks({Uri("track", 13)})).dump();
    out.write("radio-apollo-tracks", "last-page", next, ContextExpected(next));
}

Json PathfinderExpected(int status, const std::string& body, bool search) {
    SpotifyClient::WebReply reply;
    reply.status = status;
    reply.body = QByteArray::fromStdString(body);
    const QJsonObject root = QJsonDocument::fromJson(reply.body).object();
    const auto unparsed = [&reply, &root]() -> std::optional<Json> {
        if (!reply.ok()) {
            return Json::Object().set("error", Json::Object().set("status", reply.status));
        }
        if (!root.value(QStringLiteral("data")).isObject()) {
            return Json::Object().set("unreadable", true);
        }
        return std::nullopt;
    };
    if (SpotifySource::StaleQuery(reply, root)) {
        const std::optional<Json> last = unparsed();
        Check(last.has_value(), "a stale reply ends unparsed");
        return Json::Object().set("stale", true).set("withNoHashLeft", *last);
    }
    if (!reply.ok()) {
        return Json::Object().set("stale", false).set("error", Json::Object().set("status", reply.status));
    }
    Check(root.value(QStringLiteral("data")).isObject(), "a 2xx reply without data is stale");
    const QJsonObject data = root.value(QStringLiteral("data")).toObject();
    Json json = Json::Object().set("stale", false);
    if (search) {
        const std::optional<SpotifySource::Found> found = SpotifySource::FoundOf(data);
        if (!found) {
            return json.set("unreadable", true);
        }
        const char* kind = found->kind == SpotifySource::Found::Kind::Artist ? "artist"
            : found->kind == SpotifySource::Found::Kind::Album               ? "album"
            : found->kind == SpotifySource::Found::Kind::Track               ? "track"
                                                                             : "tracks";
        const Json best = found->kind == SpotifySource::Found::Kind::Tracks
            ? Json()
            : Json(Json::Object().set("uri", Std(found->best.uri)).set("name", Std(found->best.name)));
        Json artists = Json::Array();
        for (const SpotifySource::Item& artist : found->artists) {
            artists.push(Json::Object().set("uri", Std(artist.uri)).set("name", Std(artist.name)));
        }
        Json albums = Json::Array();
        for (const SpotifySource::FoundAlbum& album : found->albums) {
            albums.push(Json::Object()
                            .set("uri", Std(album.album.uri))
                            .set("name", Std(album.album.name))
                            .set("artists", Strings(album.artists))
                            .set("year", album.year ? Json(album.year) : Json())
                            .set("single", album.single));
        }
        return json.set("kind", kind)
            .set("best", best)
            .set("trackUris", Strings(found->trackUris))
            .set("artists", std::move(artists))
            .set("albums", std::move(albums));
    }
    const std::optional<QList<SpotifySource::Item>> playlists = SpotifySource::MadeForYouOf(data);
    if (!playlists) {
        return json.set("unreadable", true);
    }
    Json list = Json::Array();
    for (const SpotifySource::Item& item : *playlists) {
        list.push(Json::Object().set("uri", Std(item.uri)).set("name", Std(item.name)));
    }
    return json.set("playlists", std::move(list));
}

Json Card(const char* type, const std::string& uri, const std::string& name) {
    return Json::Object().set(
        "content",
        Json::Object()
            .set("__typename", "PlaylistResponseWrapper")
            .set("data", Json::Object().set("__typename", type).set("uri", uri).set("name", name)));
}

Json Section(const std::string& uri, const std::string& title, Json cards) {
    return Json::Object()
        .set("__typename", "HomeGenericSectionData")
        .set("uri", uri)
        .set("data", Json::Object().set("title", Json::Object().set("text", title)))
        .set("sectionItems", Json::Object().set("items", std::move(cards)).set("totalCount", 3));
}

std::string Home(Json sections) {
    return Json::Object()
        .set("data",
             Json::Object().set(
                 "home",
                 Json::Object()
                     .set("__typename", "Home")
                     .set("sectionContainer", Json::Object().set("sections", Json::Object().set("items", std::move(sections))))))
        .set("extensions", Json::Object())
        .dump();
}

void PathfinderHome(Output& out) {
    const std::string madeForYou = "spotify:section:0JQ5DAUnp4wcj0bCb3wh3S";
    const std::string other = "spotify:section:0JQ5DAnM3wGh0gz1MXnu3B";
    const Json otherSection =
        Section(other, "Recently played", Json::Array().push(Card("Playlist", Uri("playlist", 0x60), "Chill")));
    const std::string both = Home(
        Json::Array()
            .push(otherSection)
            .push(Section(madeForYou, "Made For You",
                          Json::Array()
                              .push(Card("Playlist", Uri("playlist", 0x61), "Mix 1"))
                              .push(Card("NotFound", Uri("playlist", 0x62), "DJ"))
                              .push(Card("Playlist", Uri("playlist", 0x63), "Weekly")))));
    out.write("pathfinder-home", "made-for-you", both, PathfinderExpected(200, both, false));
    const std::string without = Home(Json::Array().push(otherSection));
    out.write("pathfinder-home", "no-made-for-you", without, PathfinderExpected(200, without, false));
    const std::string container =
        Json::Object().set("data", Json::Object().set("home", Json::Object().set("__typename", "Home"))).dump();
    out.write("pathfinder-home", "no-section-container", container, PathfinderExpected(200, container, false));
}

Json Wrapped(Json data) {
    return Json::Object().set("item", Json::Object().set("data", std::move(data)));
}

Json FoundTrack(std::uint8_t mark, const std::string& name) {
    return Wrapped(Json::Object().set("__typename", "Track").set("uri", Uri("track", mark)).set("name", name));
}

std::string Search(Json top, Json tracks, std::optional<Json> artists = std::nullopt, std::optional<Json> albums = std::nullopt) {
    Json search = Json::Object()
                      .set("topResultsV2", Json::Object().set("itemsV2", std::move(top)))
                      .set("tracksV2", Json::Object().set("items", std::move(tracks)).set("totalCount", 2));
    if (artists) {
        search.set("artists", Json::Object().set("items", std::move(*artists)).set("totalCount", 3));
    }
    if (albums) {
        search.set("albumsV2", Json::Object().set("items", std::move(*albums)).set("totalCount", 4));
    }
    return Json::Object()
        .set("data", Json::Object().set("searchV2", std::move(search)))
        .set("extensions", Json::Object())
        .dump();
}

Json FoundArtist(std::uint8_t mark, const std::string& name) {
    return Json::Object()
        .set("__typename", "ArtistResponseWrapper")
        .set("data",
             Json::Object()
                 .set("__typename", "Artist")
                 .set("uri", Uri("artist", mark))
                 .set("profile", Json::Object().set("name", name)));
}

Json FoundAlbum(std::uint8_t mark, const std::string& name, const char* type, std::optional<int> year) {
    Json album = Json::Object()
                     .set("__typename", "Album")
                     .set("uri", Uri("album", mark))
                     .set("name", name)
                     .set("type", type)
                     .set("artists",
                          Json::Object().set(
                              "items",
                              Json::Array()
                                  .push(Json::Object()
                                            .set("uri", Uri("artist", 0x81))
                                            .set("profile", Json::Object().set("name", "Band")))
                                  .push(Json::Object()
                                            .set("uri", Uri("artist", 0x82))
                                            .set("profile", Json::Object().set("name", "Guest")))));
    if (year) {
        album.set("date", Json::Object().set("year", *year));
    }
    return Json::Object().set("__typename", "AlbumResponseWrapper").set("data", std::move(album));
}

void PathfinderSearch(Output& out) {
    const Json artist = Wrapped(Json::Object()
                                    .set("__typename", "Artist")
                                    .set("uri", Uri("artist", 0x81))
                                    .set("profile", Json::Object().set("name", "Band")));
    const Json album =
        Wrapped(Json::Object().set("__typename", "Album").set("uri", Uri("album", 0x71)).set("name", "Double"));
    const auto write = [&out](const std::string& name, const std::string& body, int status = 200) {
        out.write("pathfinder-searchDesktop", name, body, PathfinderExpected(status, body, true));
    };
    write("best-artist", Search(Json::Array().push(artist).push(FoundTrack(1, "Hit")),
                                Json::Array().push(FoundTrack(1, "Hit")).push(FoundTrack(2, "Found"))));
    write("best-album", Search(Json::Array().push(album), Json::Array().push(FoundTrack(3, "3")).push(FoundTrack(4, "4"))));
    write("tracks-only",
          Search(Json::Array().push(FoundTrack(2, "Found")).push(artist),
                 Json::Array()
                     .push(FoundTrack(2, "Found"))
                     .push(Json::Object().set(
                         "item", Json::Object().set("__typename", "Track").set("uri", Uri("track", 5)).set("name", "Bare")))));
    write("all-parts",
          Search(Json::Array().push(FoundTrack(1, "Hit")).push(artist),
                 Json::Array().push(FoundTrack(1, "Hit")).push(FoundTrack(2, "Found")),
                 Json::Array()
                     .push(FoundArtist(0x81, "Band"))
                     .push(FoundArtist(0x83, "Band Tribute"))
                     .push(Json::Object()
                               .set("__typename", "ArtistResponseWrapper")
                               .set("data", Json::Object().set("__typename", "NotFound").set("uri", Uri("playlist", 0x64)))),
                 Json::Array()
                     .push(FoundAlbum(0x71, "Double", "ALBUM", 1991))
                     .push(FoundAlbum(0x72, "Hit", "SINGLE", 1992))
                     .push(FoundAlbum(0x73, "Undated", "COMPILATION", std::nullopt))
                     .push(Json::Object()
                               .set("__typename", "AlbumResponseWrapper")
                               .set("data", Json::Object().set("__typename", "NotFound").set("uri", Uri("playlist", 0x65))))));
    write("nothing-found", Search(Json::Array(), Json::Array()));
    write("412-outdated", "", 412);
    write("500-with-data", Search(Json::Array().push(artist), Json::Array().push(FoundTrack(1, "Hit"))), 500);
    write("no-data",
          Json::Object()
              .set("errors", Json::Array().push(Json::Object().set("message", "PersistedQueryNotFound")))
              .dump());
}

void ApResolve(Output& out) {
    const auto write = [&out](const std::string& name, const std::string& body) {
        const QJsonObject json = QJsonDocument::fromJson(QByteArray::fromStdString(body)).object();
        QStringList accessPoints = SpotifyClient::StringsOf(json.value(QStringLiteral("accesspoint")));
        QStringList spclients = SpotifyClient::StringsOf(json.value(QStringLiteral("spclient")));
        if (accessPoints.isEmpty()) {
            accessPoints << SpotifyClient::kFallbackAccessPoint;
        }
        if (spclients.isEmpty()) {
            spclients << SpotifyClient::kFallbackSpclient;
        }
        out.write("apresolve", name, body,
                  Json::Object().set("accessPoints", Strings(accessPoints)).set("spclients", Strings(spclients)));
    };
    write("ok", Json::Object()
                    .set("accesspoint", Strings(std::vector<std::string>{"ap-gew4.spotify.com:4070",
                                                                         "ap-gew4.spotify.com:443",
                                                                         "ap-gew4.spotify.com:80"}))
                    .set("dealer", Strings(std::vector<std::string>{"gew4-dealer.spotify.com:443"}))
                    .set("spclient", Strings(std::vector<std::string>{"gew4-spclient.spotify.com:443"}))
                    .dump());
    write("empty", Json::Object().set("accesspoint", Json::Array()).set("spclient", Json::Array()).dump());
    write("503-no-body", "");
    write("blank-entries",
          Json::Object()
              .set("accesspoint", Strings(std::vector<std::string>{"", "ap-gue1.spotify.com:443"}))
              .set("spclient", Strings(std::vector<std::string>{""}))
              .dump());
}

}  // namespace

}  // namespace Vectors

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: spotify-catalog <the spec's root, where fixtures/ and expected/ are>\n";
        return 2;
    }
    Vectors::Output out(argv[1]);
    Vectors::ExtendedMetadata(out);
    Vectors::CollectionPaging(out);
    Vectors::Playlists(out);
    Vectors::StorageResolve(out);
    Vectors::Contexts(out);
    Vectors::PathfinderHome(out);
    Vectors::PathfinderSearch(out);
    Vectors::ApResolve(out);
    std::cout << argv[1] << ": " << out.count << " fixtures and their expected files\n";
    return 0;
}
