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

#ifndef CRASHREPORTER_H
#define CRASHREPORTER_H

#include <QDialog>
#include <QFile>

#include "ui_CrashReporter.h"


class CrashReporter : public QDialog
{
    Q_OBJECT

public:
    CrashReporter( const QStringList& argv );
    ~CrashReporter( );

private:
    Ui::CrashReporter ui;

    QString m_minidump;
    QString m_dir;
    QString m_product_name;
    class QHttp* m_http;

public slots:
    void send();

private slots:
    void onDone();
    void onProgress( int done, int total );
    void onFail( int error, const QString& errorString );
    void onSendButton();
};

#endif // CRASHREPORTER_H
