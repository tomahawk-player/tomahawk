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
#include "XmlQuery.h"

#include <QCoreApplication>
#include <QStringList>

using lastfm::XmlQuery;


XmlQuery::XmlQuery( const QByteArray& bytes ) throw( lastfm::ws::ParseError )
{  
    try
    {
        if ( !bytes.size() )
            throw lastfm::ws::ParseError( lastfm::ws::MalformedResponse, "No data" );

        if( !domdoc.setContent( bytes ) )
            throw lastfm::ws::ParseError( lastfm::ws::MalformedResponse, "Invalid XML" );

        e = domdoc.documentElement();

        if (e.isNull())
            throw lastfm::ws::ParseError( lastfm::ws::MalformedResponse, "Lfm is null" );

        QString const status = e.attribute( "status" );
        QDomElement error = e.firstChildElement( "error" );
        uint const n = e.childNodes().count();

        // no elements beyond the lfm is perfectably acceptable <-- wtf?
        // if (n == 0) // nothing useful in the response
        if (status == "failed" || (n == 1 && !error.isNull()) )
            throw error.isNull()
                    ? lastfm::ws::ParseError( lastfm::ws::MalformedResponse, "" )
                    : lastfm::ws::ParseError( lastfm::ws::Error( error.attribute( "code" ).toUInt() ), error.text() );

    }
    catch ( lastfm::ws::ParseError e )
    {
        switch ( e.enumValue() )
        {
            case lastfm::ws::OperationFailed:
            case lastfm::ws::InvalidApiKey:
            case lastfm::ws::InvalidSessionKey:
                // NOTE will never be received during the LoginDialog stage
                // since that happens before this slot is registered with
                // QMetaObject in App::App(). Neat :)
                QMetaObject::invokeMethod( qApp, "onWsError", Q_ARG( lastfm::ws::Error, e.enumValue() ) );
            default:
                throw e;
        }
    }
}


XmlQuery
XmlQuery::operator[]( const QString& name ) const
{
    QStringList parts = name.split( ' ' );
    if (parts.size() >= 2)
    {
        QString tagName = parts[0];
        parts = parts[1].split( '=' );
        QString attributeName = parts.value( 0 );
        QString attributeValue = parts.value( 1 );

        foreach (XmlQuery e, children( tagName ))
            if (e.e.attribute( attributeName ) == attributeValue)
                return e;
    }
    XmlQuery xq( e.firstChildElement( name ), name.toUtf8().data() );
    xq.domdoc = this->domdoc;
    return xq;
}


QList<XmlQuery>
XmlQuery::children( const QString& named ) const
{
    QList<XmlQuery> elements;
    QDomNodeList nodes = e.elementsByTagName( named );
    for (int x = 0; x < nodes.count(); ++x) {
        XmlQuery xq( nodes.at( x ).toElement() );
        xq.domdoc = this->domdoc;
        elements += xq;
    }
    return elements;
}
