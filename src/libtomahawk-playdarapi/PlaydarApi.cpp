/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#include "PlaydarApi_p.h"

#include "qxtsslserver.h"
#include "Typedefs.h"

#include "certificate/certificatebuilder.h"
#include "certificate/certificaterequestbuilder.h"
#include "certificate/keybuilder.h"
#include "utils/Logger.h"


using namespace QtAddOn::Certificate;

PlaydarApi::PlaydarApi( QHostAddress ha, qint16 port, qint16 sport, QObject* parent )
    : QObject( parent )
    , d_ptr( new PlaydarApiPrivate( this ) )
{
    Q_D( PlaydarApi );

    d->ha = ha;
    d->port = port;
    d->sport = sport;
}


PlaydarApi::~PlaydarApi()
{
}


void
PlaydarApi::start()
{
    Q_D( PlaydarApi );
    if ( !d->session.isNull() )
    {
        tLog() << "HTTPd session already exists, returning";
        return;
    }

    d->session.reset( new QxtHttpSessionManager() );
    d->connector.reset( new QxtHttpServerConnector() );
    d->tlsSession.reset( new QxtHttpSessionManager() );
    d->tlsConnector.reset( new QxtHttpsServerConnector() );
    if ( d->session.isNull() || d->connector.isNull()
         || d->tlsSession.isNull() || d->tlsConnector.isNull() )
    {
        if ( !d->session.isNull() )
            d->session.reset();
        if ( !d->connector.isNull() )
            d->connector.reset();
        if ( !d->tlsSession.isNull() )
            d->tlsSession.reset();
        if ( !d->tlsConnector.isNull() )
            d->tlsConnector.reset();
        tLog() << "Failed to start HTTPd, could not create object";
        return;
    }

    d->session->setListenInterface( d->ha );
    d->session->setPort( d->port );
    d->session->setConnector( d->connector.data() );

    d->instance.reset( new Api_v1( d->session.data() ) );
    d->session->setStaticContentService( d->instance.data() );

    tLog() << "Starting HTTPd on" << d->session->listenInterface().toString() << d->session->port();
    d->session->start();

    d->tlsSession->setListenInterface( d->ha );
    d->tlsSession->setPort( d->sport );
    d->tlsSession->setConnector( d->tlsConnector.data() );

    d->tlsInstance.reset( new Api_v1( d->tlsSession.data() ) );
    d->tlsSession->setStaticContentService( d->tlsInstance.data() );

    // Generate a SSL certificate
    QSslKey key = KeyBuilder::generate( QSsl::Rsa, KeyBuilder::StrengthNormal );

    CertificateRequestBuilder reqbuilder;
    reqbuilder.setVersion( 1 );
    reqbuilder.setKey( key );
    reqbuilder.addNameEntry( Certificate::EntryCountryName, "GB" );
    reqbuilder.addNameEntry( Certificate::EntryOrganizationName, "Tomahawk Player (Desktop)" );
    reqbuilder.addNameEntry( Certificate::EntryCommonName, "localhost" );

    // Sign the request
    CertificateRequest req = reqbuilder.signedRequest(key);

    // Now make a certificate
    CertificateBuilder builder;
    builder.setRequest( req );

    builder.setVersion( 3 );
    builder.setSerial( uuid().toLatin1() );
    builder.setActivationTime( QDateTime::currentDateTimeUtc());
    builder.setExpirationTime( QDateTime::currentDateTimeUtc().addYears( 10 ) );
    builder.setBasicConstraints( true );
    builder.addKeyPurpose( CertificateBuilder::PurposeWebServer );
    builder.setKeyUsage( CertificateBuilder::UsageCrlSign|CertificateBuilder::UsageKeyCertSign );
    builder.addSubjectKeyIdentifier();

    QSslCertificate cert = builder.signedCertificate( key );

    QxtSslServer* sslServer = d->tlsConnector->tcpServer();
    sslServer->setPrivateKey( key );
    sslServer->setLocalCertificate( cert );

    tLog() << "Starting HTTPSd on" << d->tlsSession->listenInterface().toString() << d->tlsSession->port();
    tLog() << Q_FUNC_INFO << d->tlsSession->start();
}
