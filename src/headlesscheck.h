#ifndef HEADLESSCHECK
#define HEADLESSCHECK

#ifdef ENABLE_HEADLESS

#define TOMAHAWK_APPLICATION QCoreApplication
#define TOMAHAWK_HEADLESS
#include <QCoreApplication>

#else

#define TOMAHAWK_APPLICATION QApplication
#include <QApplication>
#include "tomahawkwindow.h"

#endif

#endif
