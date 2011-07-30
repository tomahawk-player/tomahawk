-- Script to migate from db version 25 to 26.
-- Added the "autoload" column to dynamic_playlist
--


ALTER TABLE dynamic_playlist ADD COLUMN autoload BOOLEAN DEFAULT 1;

UPDATE settings SET v = '26' WHERE k == 'schema_version';
