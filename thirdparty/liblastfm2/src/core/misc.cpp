/*
   Copyright 2009 Last.fm Ltd. 
      - Primarily authored by Max Howell, Jono Cole and Doug Mansell

   This file is part of liblastfm.

   liblastfm is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   liblastfm is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with liblastfm.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "misc.h"
#include <QDir>
#ifdef WIN32
    #include <shlobj.h>
#endif
#ifdef Q_WS_MAC
    #include <Carbon/Carbon.h>
#endif


#ifdef Q_WS_MAC
QDir
lastfm::dir::bundle()
{
    // Trolltech provided example
    CFURLRef appUrlRef = CFBundleCopyBundleURL( CFBundleGetMainBundle() );
    CFStringRef macPath = CFURLCopyFileSystemPath( appUrlRef, kCFURLPOSIXPathStyle );
    QString path = CFStringToQString( macPath );
    CFRelease(appUrlRef);
    CFRelease(macPath);
    return QDir( path );
}
#endif


static QDir dataDotDot()
{
#ifdef WIN32
    if ((QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based) == 0)
    {
        // Use this for non-DOS-based Windowses
        char path[MAX_PATH];
        HRESULT h = SHGetFolderPathA( NULL, 
                                      CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE,
                                      NULL, 
                                      0, 
                                      path );
        if (h == S_OK)
            return QString::fromLocal8Bit( path );
    }
    return QDir::home();

#elif defined(Q_WS_MAC)
    return QDir::home().filePath( "Library/Application Support" );

#elif defined(Q_WS_X11)
    return QDir::home().filePath( ".local/share" );

#else
    return QDir::home();
#endif
}


QDir
lastfm::dir::runtimeData()
{
    return dataDotDot().filePath( "Last.fm" );
}


QDir
lastfm::dir::logs()
{
#ifdef Q_WS_MAC
    return QDir::home().filePath( "Library/Logs/Last.fm" );
#else
    return runtimeData();    
#endif
}


QDir
lastfm::dir::cache()
{
#ifdef Q_WS_MAC
    return QDir::home().filePath( "Library/Caches/Last.fm" );
#else
    return runtimeData().filePath( "cache" );
#endif
}


#ifdef WIN32
QDir
lastfm::dir::programFiles()
{
    char path[MAX_PATH];

    // TODO: this call is dependant on a specific version of shell32.dll.
    // Need to degrade gracefully. Need to bundle SHFolder.exe with installer
    // and execute it on install for this to work on Win98.
    HRESULT h = SHGetFolderPathA( NULL,
                                 CSIDL_PROGRAM_FILES, 
                                 NULL,
                                 0, // current path
                                 path );

    if (h != S_OK)
    {
        qCritical() << "Couldn't get Program Files dir. Possibly Win9x?";
        return QDir();
    }

    return QString::fromLocal8Bit( path );
}
#endif

#ifdef Q_WS_MAC
CFStringRef
lastfm::QStringToCFString( const QString &s )
{
    return CFStringCreateWithCharacters( 0, (UniChar*)s.unicode(), s.length() );
}

QByteArray
lastfm::CFStringToUtf8( CFStringRef s )
{
    QByteArray result;

    if (s != NULL) 
    {
        CFIndex length;
        length = CFStringGetLength( s );
        length = CFStringGetMaximumSizeForEncoding( length, kCFStringEncodingUTF8 ) + 1;
        char* buffer = new char[length];

        if (CFStringGetCString( s, buffer, length, kCFStringEncodingUTF8 ))
            result = QByteArray( buffer );
        else
            qWarning() << "CFString conversion failed.";

        delete[] buffer;
    }

    return result;
}
#endif

#if 0
// this is a Qt implementation I found
QString cfstring2qstring(CFStringRef str)
{
    if(!str)
        return QString();
    
    CFIndex length = CFStringGetLength(str);
    if(const UniChar *chars = CFStringGetCharactersPtr(str))
        return QString((QChar *)chars, length);
    UniChar *buffer = (UniChar*)malloc(length * sizeof(UniChar));
    CFStringGetCharacters(str, CFRangeMake(0, length), buffer);
    QString ret((QChar *)buffer, length);
    free(buffer);
    return ret;
}
#endif
