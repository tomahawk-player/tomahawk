-- Script to migate from db version 28 to 29.
--
-- The resulthint field used to store both servent://, file:// and all other urls
--  but all non-servent:// and file:// urls were filtered out. since we now are actually
--  using http:// resulthints, we need to weed out the 'bad ones'

-- So remoev all non-file:// or servent:// resulthints

UPDATE playlist_item SET result_hint = "" WHERE playlist_item.guid IN (SELECT guid FROM playlist_item WHERE result_hint NOT LIKE "file://%" AND result_hint NOT LIKE "servent://%");

UPDATE settings SET v = '29' WHERE k == 'schema_version';
