#ifndef HEADLESSCHECK
#define HEADLESSCHECK

#ifdef ENABLE_HEADLESS

#define TOMAHAWK_APPLICATION QtSingleCoreApplication
#define TOMAHAWK_HEADLESS
#include "qtsingleapp/qtsingleapplication.h"

#else

#define TOMAHAWK_APPLICATION QtSingleApplication
#include "qtsingleapp/qtsingleapplication.h"
#include "tomahawkwindow.h"

#endif

#endif
