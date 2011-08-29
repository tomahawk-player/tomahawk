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

#include <QX11Info>

#include <climits>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <strings.h>

#include <unistd.h>

#include <X11/Xutil.h>

#if 0
#include <X11/extensions/Xcomposite.h>
#endif

#include "netwm.h"

#define DBG(...) //fprintf(stderr, ##__VA_ARGS__)

Atom NETWM::UTF8_STRING = 0;
Atom NETWM::XROOTPMAP_ID = 0;

Atom NETWM::WM_STATE = 0;
Atom NETWM::WM_CLASS = 0;
Atom NETWM::WM_NAME = 0;
Atom NETWM::WM_DELETE_WINDOW = 0;
Atom NETWM::WM_PROTOCOLS = 0;
Atom NETWM::WM_CHANGE_STATE = 0;
Atom NETWM::WM_WINDOW_ROLE = 0;

Atom NETWM::NET_WORKAREA = 0;
Atom NETWM::NET_CLIENT_LIST = 0;
Atom NETWM::NET_CLIENT_LIST_STACKING = 0;
Atom NETWM::NET_NUMBER_OF_DESKTOPS = 0;
Atom NETWM::NET_CURRENT_DESKTOP = 0;
Atom NETWM::NET_DESKTOP_NAMES = 0;
Atom NETWM::NET_ACTIVE_WINDOW = 0;
Atom NETWM::NET_CLOSE_WINDOW = 0;
Atom NETWM::NET_SUPPORTED = 0;
Atom NETWM::NET_WM_DESKTOP = 0;
Atom NETWM::NET_SHOWING_DESKTOP = 0;

Atom NETWM::NET_WM_STATE = 0;
Atom NETWM::NET_WM_STATE_MODAL = 0;
Atom NETWM::NET_WM_STATE_STICKY = 0;
Atom NETWM::NET_WM_STATE_MAXIMIZED_VERT = 0;
Atom NETWM::NET_WM_STATE_MAXIMIZED_HORZ = 0;
Atom NETWM::NET_WM_STATE_SHADED = 0;
Atom NETWM::NET_WM_STATE_SKIP_TASKBAR = 0;
Atom NETWM::NET_WM_STATE_SKIP_PAGER = 0;
Atom NETWM::NET_WM_STATE_HIDDEN = 0;
Atom NETWM::NET_WM_STATE_FULLSCREEN = 0;
Atom NETWM::NET_WM_STATE_ABOVE = 0;
Atom NETWM::NET_WM_STATE_BELOW = 0;
Atom NETWM::NET_WM_STATE_STAYS_ON_TOP = 0;
Atom NETWM::NET_WM_STATE_STAYS_ON_BOTTOM = 0;
Atom NETWM::NET_WM_STATE_DEMANDS_ATTENTION = 0;

Atom NETWM::NET_WM_WINDOW_TYPE = 0;
Atom NETWM::NET_WM_WINDOW_TYPE_DESKTOP = 0;
Atom NETWM::NET_WM_WINDOW_TYPE_DOCK = 0;
Atom NETWM::MODERRO_WINDOW_TYPE_DOCK = 0;
Atom NETWM::NET_WM_WINDOW_TYPE_TOOLBAR = 0;
Atom NETWM::NET_WM_WINDOW_TYPE_MENU = 0;
Atom NETWM::NET_WM_WINDOW_TYPE_UTILITY = 0;
Atom NETWM::NET_WM_WINDOW_TYPE_SPLASH = 0;
Atom NETWM::NET_WM_WINDOW_TYPE_DIALOG = 0;
Atom NETWM::NET_WM_WINDOW_TYPE_DROPDOWN_MENU = 0;
Atom NETWM::NET_WM_WINDOW_TYPE_POPUP_MENU = 0;
Atom NETWM::NET_WM_WINDOW_TYPE_TOOLTIP = 0;
Atom NETWM::NET_WM_WINDOW_TYPE_NOTIFICATION = 0;
Atom NETWM::NET_WM_WINDOW_TYPE_COMBO = 0;
Atom NETWM::NET_WM_WINDOW_TYPE_DND = 0;
Atom NETWM::NET_WM_WINDOW_TYPE_NORMAL = 0;
Atom NETWM::NET_WM_WINDOW_OPACITY = 0;
Atom NETWM::NET_WM_NAME = 0;
Atom NETWM::NET_WM_VISIBLE_NAME = 0;
Atom NETWM::NET_WM_STRUT = 0;
Atom NETWM::NET_WM_STRUT_PARTIAL = 0;
Atom NETWM::NET_WM_ICON = 0;
Atom NETWM::NET_WM_PID = 0;

NETWM::net_wm_state::net_wm_state()
        : modal(0), sticky(0), maximized_vert(0),
        maximized_horz(0), shaded(0), skip_taskbar(0),
        skip_pager(0), hidden(0), fullscreen(0),
        above(0), below(0), stays_on_top(0), stays_on_bottom(0),
        demands_attention(0), valid(false)
{}

NETWM::net_wm_window_type::net_wm_window_type()
    : desktop(0), dock(0), toolbar(0),
    menu(0), utility(0), splash(0), dialog(0),
    dropdown(0), popup(0), tooltip(0), notification(0),
    combo(0), dnd(0), normal(0), valid(false)
{}

/**********************************************************/
                
void NETWM::init()
{
    Display *dpy = QX11Info::display();

    UTF8_STRING                = XInternAtom(dpy, "UTF8_STRING", False);
    XROOTPMAP_ID               = XInternAtom(dpy, "_XROOTPMAP_ID", False);
    WM_STATE                   = XInternAtom(dpy, "WM_STATE", False);
    WM_CLASS                   = XInternAtom(dpy, "WM_CLASS", False);
    WM_NAME                    = XInternAtom(dpy, "WM_NAME", False);
    WM_DELETE_WINDOW           = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    WM_CHANGE_STATE            = XInternAtom(dpy, "WM_CHANGE_STATE", False);
    WM_WINDOW_ROLE             = XInternAtom(dpy, "WM_WINDOW_ROLE", False);

    WM_PROTOCOLS               = XInternAtom(dpy, "WM_PROTOCOLS", False);
    NET_WORKAREA               = XInternAtom(dpy, "_NET_WORKAREA", False);
    NET_CLIENT_LIST            = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
    NET_CLIENT_LIST_STACKING   = XInternAtom(dpy, "_NET_CLIENT_LIST_STACKING", False);
    NET_NUMBER_OF_DESKTOPS     = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
    NET_CURRENT_DESKTOP        = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
    NET_DESKTOP_NAMES          = XInternAtom(dpy, "_NET_DESKTOP_NAMES", False);
    NET_ACTIVE_WINDOW          = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
    NET_CLOSE_WINDOW           = XInternAtom(dpy, "_NET_CLOSE_WINDOW", False);
    NET_SUPPORTED              = XInternAtom(dpy, "_NET_SUPPORTED", False);
    NET_WM_DESKTOP             = XInternAtom(dpy, "_NET_WM_DESKTOP", False);
    NET_SHOWING_DESKTOP        = XInternAtom(dpy, "_NET_SHOWING_DESKTOP", False);

    NET_WM_STATE                     = XInternAtom(dpy, "_NET_WM_STATE", False);
    NET_WM_STATE_MODAL               = XInternAtom(dpy, "_NET_WM_STATE_MODAL", False);
    NET_WM_STATE_STICKY              = XInternAtom(dpy, "_NET_WM_STATE_STICKY", False);
    NET_WM_STATE_MAXIMIZED_VERT      = XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    NET_WM_STATE_MAXIMIZED_HORZ      = XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    NET_WM_STATE_SHADED              = XInternAtom(dpy, "_NET_WM_STATE_SHADED", False);
    NET_WM_STATE_SKIP_TASKBAR        = XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", False);
    NET_WM_STATE_SKIP_PAGER          = XInternAtom(dpy, "_NET_WM_STATE_SKIP_PAGER", False);
    NET_WM_STATE_HIDDEN              = XInternAtom(dpy, "_NET_WM_STATE_HIDDEN", False);
    NET_WM_STATE_FULLSCREEN          = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
    NET_WM_STATE_ABOVE               = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);
    NET_WM_STATE_BELOW               = XInternAtom(dpy, "_NET_WM_STATE_BELOW", False);
    NET_WM_STATE_STAYS_ON_TOP        = XInternAtom(dpy, "_NET_WM_STATE_STAYS_ON_TOP", False);
    NET_WM_STATE_STAYS_ON_BOTTOM     = XInternAtom(dpy, "_NET_WM_STATE_STAYS_ON_BOTTOM", False);
    NET_WM_STATE_DEMANDS_ATTENTION   = XInternAtom(dpy, "_NET_WM_STATE_DEMANDS_ATTENTION", False);

    NET_WM_WINDOW_TYPE               = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    NET_WM_WINDOW_TYPE_DESKTOP       = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    NET_WM_WINDOW_TYPE_DOCK          = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    MODERRO_WINDOW_TYPE_DOCK         = XInternAtom(dpy, "_MODERRO_WINDOW_TYPE_DOCK", False);
    NET_WM_WINDOW_TYPE_TOOLBAR       = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
    NET_WM_WINDOW_TYPE_MENU          = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_MENU", False);
    NET_WM_WINDOW_TYPE_UTILITY       = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_UTILITY", False);
    NET_WM_WINDOW_TYPE_SPLASH        = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_SPLASH", False);
    NET_WM_WINDOW_TYPE_DIALOG        = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    NET_WM_WINDOW_TYPE_DROPDOWN_MENU = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", False);
    NET_WM_WINDOW_TYPE_POPUP_MENU    = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_POPUP_MENU", False);
    NET_WM_WINDOW_TYPE_TOOLTIP       = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_TOOLTIP", False);
    NET_WM_WINDOW_TYPE_NOTIFICATION  = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_NOTIFICATION", False);
    NET_WM_WINDOW_TYPE_COMBO         = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_COMBO", False);
    NET_WM_WINDOW_TYPE_DND           = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DND", False);
    NET_WM_WINDOW_TYPE_NORMAL        = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_NORMAL", False);

    NET_WM_WINDOW_OPACITY      = XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", False);
    NET_WM_NAME                = XInternAtom(dpy, "_NET_WM_NAME", False);
    NET_WM_VISIBLE_NAME        = XInternAtom(dpy, "_NET_WM_VISIBLE_NAME", False);
    NET_WM_STRUT               = XInternAtom(dpy, "_NET_WM_STRUT", False);
    NET_WM_STRUT_PARTIAL       = XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", False);
    NET_WM_ICON                = XInternAtom(dpy, "_NET_WM_ICON", False);
    NET_WM_PID                 = XInternAtom(dpy, "_NET_WM_PID", False);
}

int NETWM::setProperty(Window window, Atom atom, long offset, uchar *data, int nelem)
{
    NETWM::checkInit();

    return XChangeProperty(QX11Info::display(), window, atom, offset, 32, PropModeReplace, data, nelem);
}

int NETWM::setPropertySkipTaskbar(Window window)
{
    NETWM::checkInit();

    Atom state[3];

    state[0] = NETWM::NET_WM_STATE_SKIP_PAGER;
    state[1] = NETWM::NET_WM_STATE_SKIP_TASKBAR;
    state[2] = NETWM::NET_WM_STATE_STICKY;

    return NETWM::setProperty(window, NETWM::NET_WM_STATE, XA_ATOM, (uchar *)&state, 3);
}

int NETWM::setPropertyOnTop(Window window)
{
    NETWM::checkInit();

    Atom state[2];

    state[0] = NETWM::NET_WM_STATE_ABOVE;
    state[1] = NETWM::NET_WM_STATE_STAYS_ON_TOP;

    return NETWM::setProperty(window, NETWM::NET_WM_STATE, XA_ATOM, (uchar *)&state, 2);
}

void* NETWM::property(Window win, Atom prop, Atom type, int *nitems, bool *ok)
{
    NETWM::checkInit();

    Atom type_ret;
    int format_ret;
    unsigned long items_ret;
    unsigned long after_ret;
    unsigned char *prop_data = 0;

    if(XGetWindowProperty(QX11Info::display(),
                            win,
                            prop,
                            0,
                            0x7fffffff,
                            False,
                            type,
                            &type_ret,
                            &format_ret,
                            &items_ret,
                            &after_ret,
                            &prop_data) != Success)
    {
        if(ok)
            *ok = false;

        return 0;
    }

    if(nitems)
        *nitems = items_ret;

    if(ok)
        *ok = true;

    return prop_data;
}

bool NETWM::climsg(Window win, long type, long l0, long l1, long l2, long l3, long l4)
{
    NETWM::checkInit();

    XClientMessageEvent xev;

    xev.type = ClientMessage;
    xev.window = win;
    xev.message_type = type;
    xev.format = 32;
    xev.data.l[0] = l0;
    xev.data.l[1] = l1;
    xev.data.l[2] = l2;
    xev.data.l[3] = l3;
    xev.data.l[4] = l4;

    return (XSendEvent(QX11Info::display(), QX11Info::appRootWindow(), False,
            (SubstructureNotifyMask | SubstructureRedirectMask),
            (XEvent *)&xev) == Success);
}

bool NETWM::climsgwm(Window win, Atom type, Atom arg)
{
    NETWM::checkInit();

    XClientMessageEvent xev;

    xev.type = ClientMessage;
    xev.window = win;
    xev.message_type = type;
    xev.format = 32;
    xev.data.l[0] = arg;
    xev.data.l[1] = CurrentTime;

    return (XSendEvent(QX11Info::display(), win, False, 0L, (XEvent *)&xev) == Success);
}

uint NETWM::netwmDesktopsNumber()
{
    NETWM::checkInit();

    uint desknum;
    quint32 *data;

    data = (quint32 *)NETWM::property(QX11Info::appRootWindow(), NETWM::NET_NUMBER_OF_DESKTOPS, XA_CARDINAL, 0);

    if(!data)
        return 0;

    desknum = *data;
    XFree(data);

    return desknum;
}

uint NETWM::netwmCurrentDesktop()
{
    NETWM::checkInit();

    uint desk;
    quint32 *data;

    data = (quint32 *)NETWM::property(QX11Info::appRootWindow(), NETWM::NET_CURRENT_DESKTOP, XA_CARDINAL, 0);

    if(!data)
        return 0;

    desk = *data;
    XFree(data);

    return desk;
}

qint64 NETWM::netwmPid(Window win)
{
    NETWM::checkInit();

    qint64 pid = -1;
    ulong *data;

    data = (ulong *)NETWM::property(win, NETWM::NET_WM_PID, XA_CARDINAL, 0);

    if(data)
    {
        pid = *data;
        XFree(data);
    }

    return pid;
}

bool NETWM::netwmActivateWindow(Window win)
{
    NETWM::checkInit();

    return NETWM::climsg(win, NETWM::NET_ACTIVE_WINDOW, 2, CurrentTime);
}

QList<Window> NETWM::netwmWindowList()
{
    NETWM::checkInit();

    QList<Window> list;
    int num;

    Window *win = reinterpret_cast<Window *>(NETWM::property(QX11Info::appRootWindow(), NETWM::NET_CLIENT_LIST, XA_WINDOW, &num));

    if(!win)
    {
        qDebug("NETWM: Cannot get window list");
        return list;
    }

    for(int i = 0;i < num;i++)
        list.append(win[i]);

    XFree(win);

    return list;
}

int NETWM::netwmDesktop(Window win)
{
    NETWM::checkInit();

    int desk = 0;
    ulong *data;

    data = (ulong *)NETWM::property(win, NETWM::NET_WM_DESKTOP, XA_CARDINAL, 0);

    if(data)
    {
        desk = *data;
        XFree(data);
    }

    return desk;
}

NETWM::net_wm_state NETWM::netwmState(Window win)
{
    NETWM::checkInit();

    net_wm_state nws;
    Atom *state;
    int num3;

    if(!(state = (Atom *)NETWM::property(win, NETWM::NET_WM_STATE, XA_ATOM, &num3)))
        return nws;

    while(--num3 >= 0)
    {
        if(state[num3] == NETWM::NET_WM_STATE_MODAL)
        {
            DBG("NET_WM_STATE_MODAL\n");
            nws.modal = 1;
        }
        else if(state[num3] == NETWM::NET_WM_STATE_STICKY)
        {
            DBG("NET_WM_STATE_STICKY\n");
            nws.sticky = 1;
        }
        else if(state[num3] == NETWM::NET_WM_STATE_MAXIMIZED_VERT)
        {
            DBG("NET_WM_STATE_MAXIMIZED_VERT\n");
            nws.maximized_vert = 1;
        }
        else if(state[num3] == NETWM::NET_WM_STATE_MAXIMIZED_HORZ)
        {
            DBG("NET_WM_STATE_MAXIMIZED_HORZ\n");
            nws.maximized_horz = 1;
        }
        else if(state[num3] == NETWM::NET_WM_STATE_SHADED)
        {
            DBG("NET_WM_STATE_SHADED\n");
            nws.shaded = 1;
        }
        else if(state[num3] == NETWM::NET_WM_STATE_SKIP_TASKBAR)
        {
            DBG("NET_WM_STATE_SKIP_TASKBAR\n");
            nws.skip_taskbar = 1;
        }
        else if(state[num3] == NETWM::NET_WM_STATE_SKIP_PAGER)
        {
            DBG("NET_WM_STATE_SKIP_PAGER\n");
            nws.skip_pager = 1;
        }
        else if(state[num3] == NETWM::NET_WM_STATE_HIDDEN)
        {
            DBG("NET_WM_STATE_HIDDEN\n");
            nws.hidden = 1;
        }
        else if(state[num3] == NETWM::NET_WM_STATE_FULLSCREEN)
        {
            DBG("NET_WM_STATE_FULLSCREEN\n");
            nws.fullscreen = 1;
        }
        else if(state[num3] == NETWM::NET_WM_STATE_ABOVE)
        {
            DBG("NET_WM_STATE_ABOVE\n");
            nws.above = 1;
        }
        else if(state[num3] == NETWM::NET_WM_STATE_BELOW)
        {
            DBG("NET_WM_STATE_BELOW\n");
            nws.below = 1;
        }
        else if(state[num3] == NETWM::NET_WM_STATE_STAYS_ON_TOP)
        {
            DBG("NET_WM_STATE_STAYS_ON_TOP\n");
            nws.stays_on_top = 1;
        }
        else if(state[num3] == NETWM::NET_WM_STATE_STAYS_ON_BOTTOM)
        {
            DBG("NET_WM_STATE_STAYS_ON_BOTTOM\n");
            nws.stays_on_bottom = 1;
        }
        else if(state[num3] == NETWM::NET_WM_STATE_DEMANDS_ATTENTION)
        {
            DBG("NET_WM_STATE_DEMANDS_ATTENTION\n");
            nws.demands_attention = 1;
        }
    }

    nws.valid = true;

    XFree(state);

    return nws;
}

NETWM::net_wm_window_type NETWM::netwmWindowType(Window win)
{
    NETWM::checkInit();

    net_wm_window_type nwwt;
    Atom *state;
    int num3;
    bool ok;


    if(!(state = (Atom *)NETWM::property(win, NETWM::NET_WM_WINDOW_TYPE, XA_ATOM, &num3, &ok)))
    {
        if(ok)
        {
            nwwt.valid = true;
            nwwt.normal = 1;
        }

        return nwwt;
    }

    nwwt.valid = true;

    while(--num3 >= 0)
    {
        if(state[num3] == NETWM::NET_WM_WINDOW_TYPE_DESKTOP)
        {
            DBG("NET_WM_WINDOW_TYPE_DESKTOP\n");
            nwwt.desktop = 1;
        }
        else if(state[num3] == NETWM::NET_WM_WINDOW_TYPE_DOCK)
        {
            DBG("NET_WM_WINDOW_TYPE_DOCK\n");
            nwwt.dock = 1;
        }
        else if(state[num3] == NETWM::NET_WM_WINDOW_TYPE_TOOLBAR)
        {
            DBG("NET_WM_WINDOW_TYPE_TOOLBAR\n");
            nwwt.toolbar = 1;
        }
        else if(state[num3] == NETWM::NET_WM_WINDOW_TYPE_MENU)
        {
            DBG("NET_WM_WINDOW_TYPE_MENU\n");
            nwwt.menu = 1;
        }
        else if(state[num3] == NETWM::NET_WM_WINDOW_TYPE_UTILITY)
        {
            DBG("NET_WM_WINDOW_TYPE_UTILITY\n");
            nwwt.utility = 1;
        }
        else if(state[num3] == NETWM::NET_WM_WINDOW_TYPE_SPLASH)
        {
            DBG("NET_WM_WINDOW_TYPE_SPLASH\n");
            nwwt.splash = 1;
        }
        else if(state[num3] == NETWM::NET_WM_WINDOW_TYPE_DIALOG)
        {
            DBG("NET_WM_WINDOW_TYPE_DIALOG\n");
            nwwt.dialog = 1;
        }
        else if(state[num3] == NETWM::NET_WM_WINDOW_TYPE_DROPDOWN_MENU)
        {
            DBG("NET_WM_WINDOW_TYPE_DROPDOWN_MENU\n");
            nwwt.dropdown = 1;
        }
        else if(state[num3] == NETWM::NET_WM_WINDOW_TYPE_POPUP_MENU)
        {
            DBG("NET_WM_WINDOW_TYPE_POPUP_MENU\n");
            nwwt.popup = 1;
        }
        else if(state[num3] == NETWM::NET_WM_WINDOW_TYPE_TOOLTIP)
        {
            DBG("NET_WM_WINDOW_TYPE_TOOLTIP\n");
            nwwt.tooltip = 1;
        }
        else if(state[num3] == NETWM::NET_WM_WINDOW_TYPE_NOTIFICATION)
        {
            DBG("NET_WM_WINDOW_TYPE_NOTIFICATION\n");
            nwwt.notification = 1;
        }
        else if(state[num3] == NETWM::NET_WM_WINDOW_TYPE_COMBO)
        {
            DBG("NET_WM_WINDOW_TYPE_COMBO\n");
            nwwt.combo = 1;
        }
        else if(state[num3] == NETWM::NET_WM_WINDOW_TYPE_DND)
        {
            DBG("NET_WM_WINDOW_TYPE_DND\n");
            nwwt.dnd = 1;
        }
        else if(state[num3] == NETWM::NET_WM_WINDOW_TYPE_NORMAL)
        {
            DBG("NET_WM_WINDOW_TYPE_NORMAL\n");
            nwwt.normal = 1;
        }
    }

    XFree(state);

    return nwwt;
}

QString NETWM::icccmString(Window win, Atom atom)
{
    NETWM::checkInit();

    QString s;
    char *data;

    if(!(data = (char *)NETWM::property(win, atom, XA_STRING)))
        return s;

    s = QString::fromUtf8(data);

    XFree(data);

    return s;
}

QString NETWM::icccmUtf8String(Window win, Atom atom)
{
    NETWM::checkInit();

    Atom type;
    int format;
    ulong nitems;
    ulong bytes_after;
    int result;
    uchar *tmp = 0;
    QString val;

    type = None;

    result = XGetWindowProperty(QX11Info::display(), win, atom, 0, LONG_MAX, False,
                                     NETWM::UTF8_STRING, &type, &format, &nitems,
                                     &bytes_after, &tmp);

    if(result != Success || type == None || !tmp)
        return val;

    if(type == NETWM::UTF8_STRING && format == 8 && nitems != 0)
        val = QString::fromUtf8(reinterpret_cast<char *>(tmp));

    XFree(tmp);

    return val;
}

QString NETWM::icccmWindowRole(Window win)
{
    NETWM::checkInit();

    return NETWM::icccmString(win, NETWM::WM_WINDOW_ROLE);
}

QStringList NETWM::icccmClass(Window win)
{
    NETWM::checkInit();

    QStringList l;
    char *data;

    if(!(data = (char *)NETWM::property(win, NETWM::WM_CLASS, XA_STRING)))
        return l;

    l.append(QString::fromUtf8(data));
    l.append(QString::fromUtf8(data+strlen(data)+1));

    XFree(data);

    return l;
}

QString NETWM::icccmName(Window win)
{
    NETWM::checkInit();

    return NETWM::icccmString(win, NETWM::WM_NAME);
}

QStringList NETWM::icccmCommand(Window win)
{
    NETWM::checkInit();

    QStringList list;
    char **argv;
    int argc;

    if(!XGetCommand(QX11Info::display(), win, &argv, &argc))
        return list;

    for(int i = 0;i < argc;i++)
        list.append(argv[i]);

    XFreeStringList(argv);

    return list;
}

#define MO_NETWM_OPAQUE 0xffffffff

void NETWM::transset(Window window, double d)
{
    NETWM::checkInit();

    Display *dpy = QX11Info::display();

    uint opacity = (uint)(d * MO_NETWM_OPAQUE);

    if(opacity == MO_NETWM_OPAQUE)
        XDeleteProperty(dpy, window, NETWM::NET_WM_WINDOW_OPACITY);
    else
        XChangeProperty(dpy, window, NETWM::NET_WM_WINDOW_OPACITY,
                        XA_CARDINAL, 32, PropModeReplace, (uchar *)&opacity, 1L);

    XSync(dpy, False);
}

#if 0
bool NETWM::isComposite()
{
    int event_base, error_base;

    Display *dpy = QX11Info::display();

    // extension is not supported
    if(!XCompositeQueryExtension(dpy, &event_base, &error_base))
    {
        qDebug("NETWM: Composite extension is not supported");
        return false;
    }

    // NETWM-compliant composite manager MUST set selection owner
    // of _NET_WM_CM_Sn
    Window owner = XGetSelectionOwner(dpy, XInternAtom(dpy, "_NET_WM_CM_S0", False));

    return (owner != None);
}
#endif

void NETWM::checkInit()
{
    if(!NETWM::WM_STATE)
        NETWM::init();
}
