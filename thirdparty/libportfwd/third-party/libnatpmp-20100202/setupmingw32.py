#! /usr/bin/python
# $Id: setupmingw32.py,v 1.2 2009/06/04 21:37:06 nanard Exp $
# python script to build the miniupnpc module under windows
#
from distutils.core import setup, Extension
setup(name="libnatpmp", version="1.0",
      ext_modules=[
        Extension(name="libnatpmp", sources=["libnatpmpmodule.c"],
                  libraries=["ws2_32"],
                  extra_objects=["libnatpmp.a"],
                  define_macros=[('ENABLE_STRNATPMPERR', None)]
        )]
     )

