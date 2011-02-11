#ifndef QTWEETLIB_EXPORT_H
#define QTWEETLIB_EXPORT_H

#include <QtCore/QtGlobal>

#ifdef Q_WS_WIN
# define QTWEETLIB_MAKEDLL
# if defined(MAKE_QTWEETLIB_LIB)
#  define QTWEETLIB_EXPORT Q_DECL_EXPORT
# else
#  define QTWEETLIB_EXPORT Q_DECL_IMPORT
# endif
#else
# define QTWEETLIB_EXPORT Q_DECL_EXPORT
#endif

#endif

