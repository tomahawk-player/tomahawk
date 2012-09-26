-- Mutates to the database are entered into the transaction log
-- so they can be sent to peers to replay against a cache of your DB.
-- This allows peers to get diffs/sync your collection easily.

CREATE TABLE IF NOT EXISTS oplog (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    source INTEGER REFERENCES source(id) ON DELETE CASCADE ON UPDATE CASCADE, -- DEFERRABLE INITIALLY DEFERRED,
    guid TEXT NOT NULL,
    command TEXT NOT NULL,
    singleton BOOLEAN NOT NULL,
    compressed BOOLEAN NOT NULL,
    json TEXT NOT NULL
);
CREATE UNIQUE INDEX oplog_guid ON oplog(guid);
CREATE INDEX oplog_source ON oplog(source);



-- the basic 3 catalogue tables:

CREATE TABLE IF NOT EXISTS artist (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    sortname TEXT NOT NULL
);
CREATE UNIQUE INDEX artist_sortname ON artist(sortname);

CREATE TABLE IF NOT EXISTS track (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    artist INTEGER NOT NULL REFERENCES artist(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    name TEXT NOT NULL,
    sortname TEXT NOT NULL
);
CREATE UNIQUE INDEX track_artist_sortname ON track(artist,sortname);

CREATE TABLE IF NOT EXISTS album (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    artist INTEGER NOT NULL REFERENCES artist(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    name TEXT NOT NULL,
    sortname TEXT NOT NULL
);
CREATE UNIQUE INDEX album_artist_sortname ON album(artist,sortname);



-- Source, typically a remote peer.

CREATE TABLE IF NOT EXISTS source (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    friendlyname TEXT,
    lastop TEXT NOT NULL DEFAULT "",       -- guid of last op we've successfully applied
    isonline BOOLEAN NOT NULL DEFAULT false
);
CREATE UNIQUE INDEX source_name ON source(name);



-- playlists

CREATE TABLE IF NOT EXISTS playlist (
    guid TEXT PRIMARY KEY,
    source INTEGER REFERENCES source(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED, -- owner
    shared BOOLEAN DEFAULT false,
    title TEXT,
    info TEXT,
    creator TEXT,
    lastmodified INTEGER NOT NULL DEFAULT 0,
    currentrevision TEXT REFERENCES playlist_revision(guid) DEFERRABLE INITIALLY DEFERRED,
    dynplaylist BOOLEAN DEFAULT false,
    createdOn INTEGER NOT NULL DEFAULT 0
);

--INSERT INTO playlist(guid, title, info, currentrevision, dynplaylist)
--VALUES('dynamic_playlist-guid-1','Test Dynamic Playlist Dynamic','this playlist automatically created and used for testing','revisionguid-1', 1);

--INSERT INTO playlist(guid, title, info, currentrevision, dynplaylist)
--VALUES('dynamic_playlist-guid-2','Test Dynamic Playlist Static','this playlist automatically created and used for testing','revisionguid-11', 1);

CREATE TABLE IF NOT EXISTS playlist_item (
    guid TEXT PRIMARY KEY,
    playlist TEXT NOT NULL REFERENCES playlist(guid) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    trackname  TEXT NOT NULL,
    artistname TEXT NOT NULL,
    albumname  TEXT,
    annotation TEXT,
    duration INTEGER,       -- in seconds, even tho xspf uses milliseconds
    addedon INTEGER NOT NULL DEFAULT 0,   -- date added to playlist
    addedby INTEGER REFERENCES source(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED, -- who added this to the playlist
    result_hint TEXT        -- hint as to a result, to avoid using the resolver
);
CREATE INDEX playlist_item_playlist ON playlist_item(playlist);

CREATE TABLE IF NOT EXISTS playlist_revision (
    guid TEXT PRIMARY KEY,
    playlist TEXT NOT NULL REFERENCES playlist(guid) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    entries TEXT, -- qlist( guid, guid... )
    author INTEGER REFERENCES source(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    timestamp INTEGER NOT NULL DEFAULT 0,
    previous_revision TEXT REFERENCES playlist_revision(guid) DEFERRABLE INITIALLY DEFERRED
);

--INSERT INTO playlist_revision(guid, playlist, entries)
--      VALUES('revisionguid-1', 'dynamic_playlist-guid-1', '[]');
--INSERT INTO playlist_revision(guid, playlist, entries)
--      VALUES('revisionguid-11', 'dynamic_playlist-guid-2', '[]');

CREATE TABLE IF NOT EXISTS dynamic_playlist (
    guid TEXT NOT NULL REFERENCES playlist(guid) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    pltype TEXT, -- the generator type
    plmode INTEGER, -- the mode of this playlist
    autoload BOOLEAN DEFAULT true -- if this playlist should be autoloaded or not. true except for the case of special playlists we want to display elsewhere
);

--INSERT INTO dynamic_playlist(guid, pltype, plmode)
--      VALUES('dynamic_playlist-guid-1', 'echonest', 0);
--INSERT INTO dynamic_playlist(guid, pltype, plmode)
--      VALUES('dynamic_playlist-guid-2', 'echonest', 1);

-- list of controls in each playlist. each control saves a selectedType, a match, and an input
CREATE TABLE IF NOT EXISTS dynamic_playlist_controls (
    id TEXT PRIMARY KEY,
    playlist TEXT NOT NULL REFERENCES playlist(guid) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    selectedType TEXT,
    match TEXT,
    input TEXT
);

--INSERT INTO dynamic_playlist_controls(id, playlist, selectedType, match, input)
--      VALUES('controlid-1', 'dynamic_playlist-guid-1', "artist", 0, "FooArtist" );
--INSERT INTO dynamic_playlist_controls(id, playlist, selectedType, match, input)
--      VALUES('controlid-2', 'dynamic_playlist-guid-11', "artist", 0, "FooArtist" );



CREATE TABLE IF NOT EXISTS dynamic_playlist_revision (
    guid TEXT PRIMARY KEY NOT NULL REFERENCES playlist_revision(guid) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    controls TEXT, -- qlist( id, id, id )
    plmode INTEGER,
    pltype TEXT
);

--INSERT INTO dynamic_playlist_revision(guid, controls, plmode, pltype)
--      VALUES('revisionguid-1', '["controlid-1"]', 0, "echonest");
--INSERT INTO dynamic_playlist_revision(guid, controls, plmode, pltype)
--      VALUES('revisionguid-11', '["controlid-2"]', 1, "echonest");


-- files on disk and joinage with catalogue. physical properties of files only:

-- if source=null, file is local to this machine
CREATE TABLE IF NOT EXISTS file (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    source INTEGER REFERENCES source(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    url TEXT NOT NULL,                   -- file:///music/foo/bar.mp3, <guid or hash?>
    size INTEGER NOT NULL,               -- in bytes
    mtime INTEGER NOT NULL,              -- file mtime, so we know to rescan
    md5 TEXT,                            -- useful when comparing stuff p2p
    mimetype TEXT,                       -- "audio/mpeg"
    duration INTEGER NOT NULL DEFAULT 0, -- seconds
    bitrate INTEGER NOT NULL DEFAULT 0   -- kbps (or equiv)
);
CREATE UNIQUE INDEX file_url_src_uniq ON file(source, url);
CREATE INDEX file_source ON file(source);
CREATE INDEX file_mtime ON file(mtime);

-- mtime of dir when last scanned.
-- load into memory when rescanning, skip stuff that's unchanged
CREATE TABLE IF NOT EXISTS dirs_scanned (
    name TEXT PRIMARY KEY,
    mtime INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS file_join (
    file INTEGER PRIMARY KEY REFERENCES file(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    artist INTEGER NOT NULL REFERENCES artist(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    track INTEGER NOT NULL REFERENCES track(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    album INTEGER REFERENCES album(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    albumpos INTEGER,
    composer INTEGER REFERENCES artist(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    discnumber INTEGER
);
CREATE INDEX file_join_track  ON file_join(track);
CREATE INDEX file_join_artist ON file_join(artist);
CREATE INDEX file_join_album  ON file_join(album);



-- tags, weighted and by source (rock, jazz etc)

-- weight is always 1.0 if tag provided by our user.
-- may be less from aggregate sources like lastfm global tags
CREATE TABLE IF NOT EXISTS track_tags (
    id INTEGER PRIMARY KEY,   -- track id
    source INTEGER REFERENCES source(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    tag TEXT NOT NULL,        -- always store as lowercase
    ns TEXT,                  -- ie 'last.fm', 'echonest'
    weight float DEFAULT 1.0  -- range 0-1
);
CREATE INDEX track_tags_tag ON track_tags(tag);

CREATE TABLE IF NOT EXISTS album_tags (
    id INTEGER PRIMARY KEY,   -- album id
    source INTEGER REFERENCES source(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    tag TEXT NOT NULL,        -- always store as lowercase
    ns TEXT,                  -- ie 'last.fm', 'echonest'
    weight float DEFAULT 1.0  -- range 0-1
);
CREATE INDEX album_tags_tag ON album_tags(tag);

CREATE TABLE IF NOT EXISTS artist_tags (
    id INTEGER PRIMARY KEY,   -- artist id
    source INTEGER REFERENCES source(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    tag TEXT NOT NULL,        -- always store as lowercase
    ns TEXT,                  -- ie 'last.fm', 'echonest'
    weight float DEFAULT 1.0  -- range 0-1
);
CREATE INDEX artist_tags_tag ON artist_tags(tag);

-- all other attributes.
-- like tags that have a value, eg:
--  BPM=120, releaseyear=1980, key=Dminor, composer=Someone
-- NB: since all values are text, numeric values should be zero-padded to a set amount
--     so that we can always do range queries.

CREATE TABLE IF NOT EXISTS track_attributes (
    id INTEGER REFERENCES track(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,   -- track id
    k TEXT NOT NULL,
    v TEXT NOT NULL
);
CREATE INDEX track_attrib_id ON track_attributes(id);
CREATE INDEX track_attrib_k  ON track_attributes(k);

-- Collection attributes, tied to a source. An example might be an echonest song catalog

CREATE TABLE IF NOT EXISTS collection_attributes (
    id INTEGER REFERENCES source(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED, -- source id, null for local source
    k TEXT NOT NULL,
    v TEXT NOT NULL
);

-- social attributes connected to the track.
-- like love, hate, comments, recommendations
--  love=[comment], hate=[comment], comment=Some text
-- NB: since all values are text, numeric values should be zero-padded to a set amount
--     so that we can always do range queries.

CREATE TABLE IF NOT EXISTS social_attributes (
    id INTEGER REFERENCES track(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,   -- track id
    source INTEGER REFERENCES source(id) ON DELETE CASCADE ON UPDATE CASCADE, -- DEFERRABLE INITIALLY DEFERRED,
    k TEXT NOT NULL,
    v TEXT NOT NULL,
    timestamp INTEGER NOT NULL DEFAULT 0
);
CREATE INDEX social_attrib_id        ON social_attributes(id);
CREATE INDEX social_attrib_source    ON social_attributes(source);
CREATE INDEX social_attrib_k         ON social_attributes(k);
CREATE INDEX social_attrib_timestamp ON social_attributes(timestamp);



-- playback history

-- if source=null, file is local to this machine
CREATE TABLE IF NOT EXISTS playback_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    source INTEGER REFERENCES source(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    track INTEGER REFERENCES track(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    playtime INTEGER NOT NULL,              -- when playback finished (timestamp)
    secs_played INTEGER NOT NULL
);

CREATE INDEX playback_log_source ON playback_log(source);
CREATE INDEX playback_log_track ON playback_log(track);



-- auth information for http clients

CREATE TABLE IF NOT EXISTS http_client_auth (
    token TEXT NOT NULL PRIMARY KEY,
    website TEXT NOT NULL,
    name TEXT NOT NULL,
    ua TEXT,
    mtime INTEGER,
    permissions TEXT NOT NULL
);



-- Schema version, and misc tomahawk settings relating to the collection db

CREATE TABLE IF NOT EXISTS settings (
    k TEXT NOT NULL PRIMARY KEY,
    v TEXT NOT NULL DEFAULT ''
);

INSERT INTO settings(k,v) VALUES('schema_version', '29');
