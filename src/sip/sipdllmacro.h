#ifndef SIPDLLMACRO_H
#define SIPDLLMACRO_H

#ifdef WIN32
  #ifdef SIPDLLEXPORT_PRO
      #define SIPDLLEXPORT __declspec(dllexport)
  #else
      #define SIPDLLEXPORT __declspec(dllimport)
  #endif
#else
  #define SIPDLLEXPORT
#endif

#endif
