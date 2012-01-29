-- Script to migate from db version 27 to 28.
-- Added albumartist and discnumber to file_join

ALTER TABLE file_join ADD COLUMN composer INTEGER REFERENCES artist(id) ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED;
ALTER TABLE file_join ADD COLUMN discnumber INTEGER;

UPDATE settings SET v = '28' WHERE k == 'schema_version';
