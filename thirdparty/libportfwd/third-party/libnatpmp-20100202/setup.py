#! /usr/bin/python
# $Id: setup.py,v 1.2 2009/06/04 21:37:06 nanard Exp $
#
# python script to build the libnatpmp module under unix
#
# replace libnatpmp.a by libnatpmp.so for shared library usage
from distutils.core import setup, Extension
setup(name="libnatpmp", version="1.0",
      ext_modules=[
        Extension(name="libnatpmp", sources=["libnatpmpmodule.c"],
                  extra_objects=["libnatpmp.a"],
                  define_macros=[('ENABLE_STRNATPMPERR', None)]
        )]
     )

