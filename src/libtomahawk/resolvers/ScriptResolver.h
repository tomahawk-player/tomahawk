/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi            <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac           <teo@kde.org>
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

#include "Query.h"
#include "Artist.h"
#include "Album.h"
#include "collection/Collection.h"
#include "ExternalResolverGui.h"
#include "DllMacro.h"

#include <QProcess>

class QWidget;

namespace Tomahawk
{

class DLLEXPORT ScriptResolver : public Tomahawk::ExternalResolverGui
{
Q_OBJECT

public:
    explicit ScriptResolver( const QString& exe );
    virtual ~ScriptResolver();
    static ExternalResolver* factory( const QString& accountId, const QString& exe, const QStringList& );

    QString name() const Q_DECL_OVERRIDE { return m_name; }
    QPixmap icon( const QSize& size ) const Q_DECL_OVERRIDE;
    unsigned int weight() const Q_DECL_OVERRIDE { return m_weight; }
    virtual unsigned int preference() const { return m_preference; }
    unsigned int timeout() const Q_DECL_OVERRIDE { return m_timeout; }
    Capabilities capabilities() const Q_DECL_OVERRIDE { return m_capabilities; }

    void setIcon( const QPixmap& icon ) Q_DECL_OVERRIDE;

    AccountConfigWidget* configUI() const Q_DECL_OVERRIDE;
    void saveConfig() Q_DECL_OVERRIDE;

    ExternalResolver::ErrorState error() const Q_DECL_OVERRIDE;
    void reload() Q_DECL_OVERRIDE;

    bool running() const Q_DECL_OVERRIDE;

    void sendMessage( const QVariantMap& map );

    bool canParseUrl( const QString&, UrlType ) Q_DECL_OVERRIDE { return false; }

signals:
    void terminated();
    void customMessage( const QString& msgType, const QVariantMap& msg );

public slots:
    void stop() Q_DECL_OVERRIDE;
    void resolve( const Tomahawk::query_ptr& query ) Q_DECL_OVERRIDE;
    void start() Q_DECL_OVERRIDE;

    // TODO: implement. Or not. Not really an issue while Spotify doesn't do browsable personal cloud storage.
    void artists( const Tomahawk::collection_ptr& ) Q_DECL_OVERRIDE {}
    void albums( const Tomahawk::collection_ptr&, const Tomahawk::artist_ptr& ) Q_DECL_OVERRIDE {}
    void tracks( const Tomahawk::collection_ptr&, const Tomahawk::album_ptr& ) Q_DECL_OVERRIDE {}
    void lookupUrl( const QString&  ) Q_DECL_OVERRIDE {}


private slots:
    void readStderr();
    void readStdout();
    void cmdExited( int code, QProcess::ExitStatus status );

private:
    void sendConfig();

    void handleMsg( const QByteArray& msg );
    void sendMsg( const QByteArray& msg );
    void doSetup( const QVariantMap& m );
    void setupConfWidget( const QVariantMap& m );

    void startProcess();

    QProcess m_proc;
    QString m_name;
    QPixmap m_icon;
    unsigned int m_weight, m_preference, m_timeout, m_num_restarts;
    Capabilities m_capabilities;
    QPointer< AccountConfigWidget > m_configWidget;

    quint32 m_msgsize;
    QByteArray m_msg;

    bool m_ready, m_stopped, m_configSent, m_deleting;
    ExternalResolver::ErrorState m_error;
};

}

#endif // SCRIPTRESOLVER_H
