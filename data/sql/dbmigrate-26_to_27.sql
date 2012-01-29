-- Script to migate from db version 26 to 27
-- Nothing to do

CREATE TABLE IF NOT EXISTS collection_attributes (
    id INTEGER REFERENCES source(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED, -- source id, null for local source
    k TEXT NOT NULL,
    v TEXT NOT NULL
);
UPDATE settings SET v = '27' WHERE k == 'schema_version';

