To query or modify the database you must use a DatabaseCommand.
The DatabaseCommand objects are processed sequentially and asynchronously
by the DatabaseWorker.

This means you need to dispatch the cmd, and connect to a finished signal.
There are no blocking DB calls in the application code, except in the 
exec() method of a DatabaseCommand object.

If you inherit DatabaseCommandLoggable, the command is serialized into the
oplog, so that peers can replay it against their cache of your database.

For example, if you dispatch an addTracks DBCmd, after scanning a new album,
this will be serialized, and peers will replay it so that their cached version
of your collection is kept up to date.

DBCmds have GUIDs, and are ordered by the 'id' in the oplog table.

The last DBCmd GUID applied to your cache of a source's collection is stored
in the source table (the lastop field). 

The DBSyncConnection will ask for all ops newer than that GUID, and replay.

