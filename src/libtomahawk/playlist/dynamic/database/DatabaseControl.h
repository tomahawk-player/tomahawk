/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef DATABASE_CONTROL_H
#define DATABASE_CONTROL_H

#include "playlist/dynamic/DynamicControl.h"

#include <QTimer>

namespace Tomahawk
{

    class DatabaseControl : public DynamicControl
    {
        Q_OBJECT
    public:
        virtual QWidget* inputField();
        virtual QWidget* matchSelector();

        virtual QString input() const;
        virtual QString match() const;
        virtual QString matchString() const;
        virtual QString summary() const;

        virtual void setInput(const QString& input);
        virtual void setMatch(const QString& match);

        /// DO NOT USE IF YOU ARE NOT A DBCMD
        DatabaseControl( const QString& type, const QStringList& typeSelectors, QObject* parent = 0 );
        DatabaseControl( const QString& sql, const QString& summary, const QStringList& typeSelectors, QObject* parent = 0 );

        QString sql() const;

    public slots:
        virtual void setSelectedType ( const QString& type );

    private slots:
        void updateData();
        void editingFinished();
        void editTimerFired();

    private:
        void updateWidgets();
        void updateWidgetsFromData();

        // utility
        void calculateSummary();

        QPointer< QWidget > m_input;
        QPointer< QWidget > m_match;
        QString m_matchData;
        QString m_matchString;
        QString m_summary;

        QTimer m_editingTimer;
        QTimer m_delayedEditTimer;

        // SQL control
        QString m_sql;
        QString m_sqlSummary;
    };

};

#endif
