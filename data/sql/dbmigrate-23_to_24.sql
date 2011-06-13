-- Script to migate from db version 23 to 24.
-- Added the social_attributes table.
--
-- Separate each command with %%

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

UPDATE settings SET v = '24' WHERE k == 'schema_version';
