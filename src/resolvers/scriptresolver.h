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

#ifndef SCRIPTRESOLVER_H
#define SCRIPTRESOLVER_H

#include <QProcess>

#include <qjson/parser.h>
#include <qjson/serializer.h>
#include <qjson/qobjecthelper.h>

#include "resolver.h"
#include "query.h"
#include "result.h"

class ScriptResolver : public Tomahawk::ExternalResolver
{
Q_OBJECT

public:
    explicit ScriptResolver( const QString& exe );
    virtual ~ScriptResolver();

    virtual QString name() const            { return m_name; }
    virtual unsigned int weight() const     { return m_weight; }
    virtual unsigned int preference() const { return m_preference; }
    virtual unsigned int timeout() const    { return m_timeout; }

signals:
    void finished();

public slots:
    virtual void stop();
    virtual void resolve( const Tomahawk::query_ptr& query );

private slots:
    void readStderr();
    void readStdout();
    void cmdExited( int code, QProcess::ExitStatus status );

    void onTimeout( const Tomahawk::query_ptr& query );

private:
    void handleMsg( const QByteArray& msg );
    void sendMsg( const QByteArray& msg );
    void doSetup( const QVariantMap& m );

    QProcess m_proc;
    QString m_name;
    unsigned int m_weight, m_preference, m_timeout, m_num_restarts;

    quint32 m_msgsize;
    QByteArray m_msg;

    bool m_ready, m_stopped;

    QHash< QString /* QID */, unsigned int /* state */ > m_queryState;

    QJson::Parser m_parser;
    QJson::Serializer m_serializer;
};

#endif // SCRIPTRESOLVER_H
