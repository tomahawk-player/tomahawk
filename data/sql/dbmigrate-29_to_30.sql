-- Script to migate from db version 29 to 30.

-- Add the following index to speed up all Operations that deal with a specific time period of plays
CREATE INDEX playback_log_playtime ON playback_log(playtime);

UPDATE settings SET v = '30' WHERE k == 'schema_version';
