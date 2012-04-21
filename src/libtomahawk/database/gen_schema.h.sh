#!/bin/bash

# !!!! You need to manually generate Schema.sql.h when the schema changes:
# ./gen_schema.h.sh ./Schema.sql tomahawk > Schema.sql.h

schema=$1
name=$2

if [ -e "$schema" -a -n "$name" ]
then
cat <<EOF
/*
    This file was automatically generated from $schema on `date`.
*/

static const char * ${name}_schema_sql = 
EOF
    awk '!/^-/ && length($0) {gsub(/[ \t]+$/, "", $0); gsub("\"","\\\"",$0); gsub("--.*$","",$0); printf("\"%s\"\n",$0);}' "$schema"
cat <<EOF
    ;

const char * get_${name}_sql()
{
    return ${name}_schema_sql;
}


EOF
else
    echo "Usage: $0 <Schema.sql> <name>"
    exit 1
fi
