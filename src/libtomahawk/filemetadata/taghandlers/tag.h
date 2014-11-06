/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Teo Mrnjavac <teo@kde.org>
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

#ifndef TAG_H
#define TAG_H

#include "DllMacro.h"

#include <QSharedPointer>
#include <QtCore/QString>

#include <taglib/tag.h>
#include <taglib/fileref.h>

namespace Tomahawk
{

class DLLEXPORT Tag
{
public:
    static QSharedPointer<Tag> fromFile( const TagLib::FileRef &f );

    //getter-setters for common TagLib items
    QString title() const { return TStringToQString( m_tag->title() ).trimmed(); }
    QString artist() const { return TStringToQString( m_tag->artist() ).trimmed(); }
    QString album() const { return TStringToQString( m_tag->album() ).trimmed(); }
    QString comment() const { return TStringToQString( m_tag->comment() ).trimmed(); }
    QString genre() const { return TStringToQString( m_tag->genre() ).trimmed(); }
    unsigned int year() const { return m_tag->year(); }
    unsigned int track() const { return m_tag->track(); }
    void setTitle( const QString &s ) { m_tag->setTitle( TagLib::String( s.toUtf8().data(), TagLib::String::UTF8 ) ); }
    void setArtist( const QString &s ) { m_tag->setArtist( TagLib::String( s.toUtf8().data(), TagLib::String::UTF8 ) ); }
    void setAlbum( const QString &s ) { m_tag->setAlbum( TagLib::String( s.toUtf8().data(), TagLib::String::UTF8 ) ); }
    void setComment( const QString &s ) { m_tag->setComment( TagLib::String( s.toUtf8().data(), TagLib::String::UTF8 ) ); }
    void setGenre( const QString &s ) { m_tag->setGenre( TagLib::String( s.toUtf8().data(), TagLib::String::UTF8 ) ); }
    void setYear( unsigned int i ) { m_tag->setYear( i ); }
    void setTrack( unsigned int i ) { m_tag->setTrack( i ); }
    bool isEmpty() const { return m_tag->isEmpty(); }

    QString albumArtist() const { return m_albumArtist; }
    QString composer() const { return m_composer; }
    unsigned int discNumber() const { return m_discNumber; }
    //TODO: add support for writing those 3 items with TagLib's addField/setField

protected:
    Tag( TagLib::Tag *tag ) : m_tag( tag ), m_discNumber( 0 ) {}

    unsigned int processDiscNumber( const QString & ) const;

    TagLib::Tag *m_tag;
    QString m_albumArtist;
    QString m_composer;
    unsigned int m_discNumber;
};

}

#endif // TAG_H
