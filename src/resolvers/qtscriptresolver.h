/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef QTSCRIPTRESOLVER_H
#define QTSCRIPTRESOLVER_H

#include "resolver.h"
#include "query.h"
#include "result.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QThread>
#include <QtWebKit/QWebPage>
#include <QtWebKit/QWebFrame>

class QtScriptResolver;

class ScriptEngine : public QWebPage
{
Q_OBJECT

public:
    explicit ScriptEngine( QtScriptResolver* parent )
        : QWebPage( (QObject*) parent )
        , m_parent( parent )
    {
    }

public slots:
    bool shouldInterruptJavaScript()
    {
        return true;
    }

protected:
    virtual void javaScriptConsoleMessage( const QString & message, int lineNumber, const QString & sourceID )
    { qDebug() << "JAVASCRIPT ERROR:" << message << lineNumber << sourceID; }

private:
    QtScriptResolver* m_parent;
};


class QtScriptResolver : public Tomahawk::ExternalResolver
{
Q_OBJECT

public:
    explicit QtScriptResolver( const QString& scriptPath );
    virtual ~QtScriptResolver();

    virtual QString name() const            { return m_name; }
    virtual unsigned int weight() const     { return m_weight; }
    virtual unsigned int timeout() const    { return m_timeout; }

    virtual QWidget* configUI() const { return 0; } // TODO support properly for qtscript resolvers too!
    virtual void saveConfig() {}
public slots:
    virtual void resolve( const Tomahawk::query_ptr& query );
    virtual void stop();

signals:
    void finished();

private:
    ScriptEngine* m_engine;

    QString m_name;
    unsigned int m_weight, m_timeout;

    bool m_ready, m_stopped;
};

#endif // QTSCRIPTRESOLVER_H
