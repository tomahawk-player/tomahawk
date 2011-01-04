#ifndef DLLMACRO_H
#define DLLMACRO_H

#ifdef WIN32
  #ifdef DLLEXPORT_PRO
      #define DLLEXPORT __declspec(dllexport)
  #else
      #define DLLEXPORT __declspec(dllimport)
  #endif
#else
  #define DLLEXPORT
#endif

#endif
