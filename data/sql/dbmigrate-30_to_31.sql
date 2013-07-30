-- Script to migate from db version 30 to 31.

-- Better indices to join playlist_item with other tables
CREATE INDEX playlist_item_trackname ON playlist_item(trackname);
CREATE INDEX playlist_item_artistname ON playlist_item(artistname);
CREATE INDEX artist_name ON artist(name);
CREATE INDEX track_name ON track(name);

UPDATE settings SET v = '31' WHERE k == 'schema_version';
