-- Script to migate from db version 24 to 25.
-- Added the social_attributes table.
--

ALTER TABLE dynamic_playlist RENAME TO tmp_dynamic_playlist;

CREATE TABLE IF NOT EXISTS dynamic_playlist (
    guid TEXT NOT NULL REFERENCES playlist(guid) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED,
    pltype TEXT, -- the generator type
    plmode INTEGER -- the mode of this playlist
);

INSERT INTO dynamic_playlist( guid, pltype, plmode ) SELECT guid, pltype, plmode FROM tmp_dynamic_playlist;

DROP TABLE tmp_dynamic_playlist;

UPDATE settings SET v = '25' WHERE k == 'schema_version';
