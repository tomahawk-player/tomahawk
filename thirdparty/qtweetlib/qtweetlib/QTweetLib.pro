TEMPLATE = subdirs
SUBDIRS = sub_qjson sub_src sub_examples

sub_qjson.subdir = qjson

sub_src.subdir = src
sub_src.depends = sub_qjson

sub_examples.subdir = examples
sub_examples.depends = sub_src
