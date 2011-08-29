/***************************************************************************
 *   Copyright (C) 2010 by Dmitry 'Krasu' Baryshev                         *
 *   ksquirrel.iv@gmail.com                                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef NETWM_H
#define NETWM_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "fixx11h.h"

#include <QStringList>
#include <QString>
#include <QList>

class NETWM
{

public:
    struct net_wm_state
    {
        net_wm_state();

        unsigned int modal             : 1;
        unsigned int sticky            : 1;
        unsigned int maximized_vert    : 1;
        unsigned int maximized_horz    : 1;
        unsigned int shaded            : 1;
        unsigned int skip_taskbar      : 1;
        unsigned int skip_pager        : 1;
        unsigned int hidden            : 1;
        unsigned int fullscreen        : 1;
        unsigned int above             : 1;
        unsigned int below             : 1;
        unsigned int stays_on_top      : 1;
        unsigned int stays_on_bottom   : 1;
        unsigned int demands_attention : 1;
        bool valid;
    };

    struct net_wm_window_type
    {
        net_wm_window_type();

        unsigned int desktop      : 1;
        unsigned int dock         : 1;
        unsigned int toolbar      : 1;
        unsigned int menu         : 1;
        unsigned int utility      : 1;
        unsigned int splash       : 1;
        unsigned int dialog       : 1;
        unsigned int dropdown     : 1;
        unsigned int popup        : 1;
        unsigned int tooltip      : 1;
        unsigned int notification : 1;
        unsigned int combo        : 1;
        unsigned int dnd          : 1;
        unsigned int normal       : 1;
        bool valid;
    };

    /*************************************************************************/

    static void init();

    static void transset(Window, double);

#if 0
    static bool isComposite();
#endif

    static int setProperty(Window, Atom, long, uchar *, int);

    static int setPropertySkipTaskbar(Window);
    static int setPropertyOnTop(Window);

    static void* property(Window win, Atom prop, Atom type, int *nitems = 0, bool *ok = 0);

    static bool climsg(Window win, long type, long l0, long l1 = 0, long l2 = 0, long l3 = 0, long l4 = 0);
    static bool climsgwm(Window win, Atom type, Atom arg);

    // NETWM helper functions
    static qint64 netwmPid(Window win);
    static QList<Window> netwmWindowList();
    static uint netwmDesktopsNumber();
    static uint netwmCurrentDesktop();
    static int netwmDesktop(Window win);
    static net_wm_state netwmState(Window win);
    static net_wm_window_type netwmWindowType(Window win);
    static bool netwmActivateWindow(Window win);

    // ICCCM helper functions
    static QString icccmString(Window win, Atom atom);
    static QString icccmUtf8String(Window win, Atom atom);
    static QString icccmWindowRole(Window win);
    static QStringList icccmClass(Window win);
    static QString icccmName(Window win);
    static QStringList icccmCommand(Window win);

    /*************************************************************************/
    /********************************* Atoms *********************************/
    /*************************************************************************/

    static Atom UTF8_STRING;
    static Atom XROOTPMAP_ID;

    static Atom WM_STATE;
    static Atom WM_CLASS;
    static Atom WM_NAME;
    static Atom WM_DELETE_WINDOW;
    static Atom WM_PROTOCOLS;
    static Atom WM_CHANGE_STATE;
    static Atom WM_WINDOW_ROLE;

    static Atom NET_WORKAREA;
    static Atom NET_CLIENT_LIST;
    static Atom NET_CLIENT_LIST_STACKING;
    static Atom NET_NUMBER_OF_DESKTOPS;
    static Atom NET_CURRENT_DESKTOP;
    static Atom NET_DESKTOP_NAMES;
    static Atom NET_ACTIVE_WINDOW;
    static Atom NET_CLOSE_WINDOW;
    static Atom NET_SUPPORTED;
    static Atom NET_WM_DESKTOP;
    static Atom NET_SHOWING_DESKTOP;

    static Atom NET_WM_STATE;
    static Atom NET_WM_STATE_MODAL;
    static Atom NET_WM_STATE_STICKY;
    static Atom NET_WM_STATE_MAXIMIZED_VERT;
    static Atom NET_WM_STATE_MAXIMIZED_HORZ;
    static Atom NET_WM_STATE_SHADED;
    static Atom NET_WM_STATE_SKIP_TASKBAR;
    static Atom NET_WM_STATE_SKIP_PAGER;
    static Atom NET_WM_STATE_HIDDEN;
    static Atom NET_WM_STATE_FULLSCREEN;
    static Atom NET_WM_STATE_ABOVE;
    static Atom NET_WM_STATE_BELOW;
    static Atom NET_WM_STATE_STAYS_ON_TOP;
    static Atom NET_WM_STATE_STAYS_ON_BOTTOM;
    static Atom NET_WM_STATE_DEMANDS_ATTENTION;

    static Atom NET_WM_WINDOW_TYPE;
    static Atom NET_WM_WINDOW_TYPE_DESKTOP;
    static Atom NET_WM_WINDOW_TYPE_DOCK;
    static Atom MODERRO_WINDOW_TYPE_DOCK;
    static Atom NET_WM_WINDOW_TYPE_TOOLBAR;
    static Atom NET_WM_WINDOW_TYPE_MENU;
    static Atom NET_WM_WINDOW_TYPE_UTILITY;
    static Atom NET_WM_WINDOW_TYPE_SPLASH;
    static Atom NET_WM_WINDOW_TYPE_DIALOG;
    static Atom NET_WM_WINDOW_TYPE_DROPDOWN_MENU;
    static Atom NET_WM_WINDOW_TYPE_POPUP_MENU;
    static Atom NET_WM_WINDOW_TYPE_TOOLTIP;
    static Atom NET_WM_WINDOW_TYPE_NOTIFICATION;
    static Atom NET_WM_WINDOW_TYPE_COMBO;
    static Atom NET_WM_WINDOW_TYPE_DND;
    static Atom NET_WM_WINDOW_TYPE_NORMAL;
    static Atom NET_WM_WINDOW_OPACITY;
    static Atom NET_WM_NAME;
    static Atom NET_WM_VISIBLE_NAME;
    static Atom NET_WM_STRUT;
    static Atom NET_WM_STRUT_PARTIAL;
    static Atom NET_WM_ICON;
    static Atom NET_WM_PID;

private:
    static void checkInit();
};

#endif
