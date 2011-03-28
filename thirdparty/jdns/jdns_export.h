#ifndef JDNS_EXPORT_H
#define JDNS_EXPORT_H

#include <QtCore/QtGlobal>

#ifdef Q_WS_WIN
# if defined(MAKE_JDNS_LIB)
#  define JDNS_EXPORT Q_DECL_EXPORT
# else
#  define JDNS_EXPORT Q_DECL_IMPORT
# endif
#else
# define JDNS_EXPORT Q_DECL_EXPORT
#endif

#endif

