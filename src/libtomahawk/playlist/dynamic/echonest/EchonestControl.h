/****************************************************************************************
 * Copyright (c) 2010 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef ECHONEST_CONTROL_H
#define ECHONEST_CONTROL_H

#include <echonest/Playlist.h>

#include "dynamic/DynamicControl.h"
#include <QTimer>

namespace Tomahawk
{
    
class EchonestControl : public DynamicControl
{
    Q_OBJECT
public:
    virtual QWidget* inputField();
    virtual QWidget* matchSelector();
    
    /// Converts this to an echonest suitable parameter
    Echonest::DynamicPlaylist::PlaylistParamData toENParam() const;
    
    virtual QString input() const;
    virtual QString match() const;
    virtual QString matchString() const;
    virtual QString summary() const;
    
    virtual void setInput(const QString& input);
    virtual void setMatch(const QString& match);
    
    /// DO NOT USE IF YOU ARE NOT A DBCMD
    EchonestControl( const QString& type, const QStringList& typeSelectors, QObject* parent = 0 );
    
public slots:
    virtual void setSelectedType ( const QString& type );
        
private slots:
    void updateData();
    void editingFinished();
    
private:
    void updateWidgets();
    void updateWidgetsFromData();
    
    // utility
    void setupMinMaxWidgets( Echonest::DynamicPlaylist::PlaylistParam min, Echonest::DynamicPlaylist::PlaylistParam max, const QString& leftL, const QString& rightL, int maxRange );
    void updateFromComboAndSlider( bool smooth = false );
    void updateFromLabelAndCombo();
    
    void updateToComboAndSlider( bool smooth = false );
    void updateToLabelAndCombo();
    
    void calculateSummary();
    
    Echonest::DynamicPlaylist::PlaylistParam m_currentType;
    int m_overrideType;
    
    QWeakPointer< QWidget > m_input;
    QWeakPointer< QWidget > m_match;
    QString m_matchData;
    QString m_matchString;
    QString m_summary;
    
    QTimer m_editingTimer;
    
    Echonest::DynamicPlaylist::PlaylistParamData m_data;
    
    friend class EchonestGenerator;
};

};

#endif
