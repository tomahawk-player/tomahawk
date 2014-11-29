/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include "DllMacro.h"

#include <QSslError>
#include <QWebPage>

class QNetworkReply;

namespace Tomahawk
{

class JSPlugin;

class DLLEXPORT ScriptEngine : public QWebPage
{
Q_OBJECT

public:
    explicit ScriptEngine( JSPlugin* parent );

    QString userAgentForUrl( const QUrl& url ) const;
    void setScriptPath( const QString& scriptPath );

public slots:
    bool shouldInterruptJavaScript();

protected:
    virtual void javaScriptConsoleMessage( const QString& message, int lineNumber, const QString& sourceID );

private slots:
    void sslErrorHandler( QNetworkReply* qnr, const QList<QSslError>& errlist );

private:
    JSPlugin* m_parent;
    QString m_scriptPath;
    QString m_header;
};

} // ns: Tomahawk

#endif // SCRIPTENGINE_H
