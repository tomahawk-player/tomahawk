-- Script to migate from db version 22 to 23.
-- Only change in this version is that playlists gained a createdOn date.
-- Set all playlists to created to now.
--
-- Separate each command with %%

ALTER TABLE playlist ADD COLUMN createdOn INTEGER NOT NULL DEFAULT 0;

UPDATE playlist SET createdOn = strftime( '%s','now' );

UPDATE settings SET v = '23' WHERE k == 'schema_version';
