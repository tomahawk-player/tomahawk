/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Leo Franchi <lfranchi@kde.org>
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
#ifndef BINARY_INSTALLER_HELPER
#define BINARY_INSTALLER_HELPER

#include "AtticaManager.h"

#include <QPointer>

class QTemporaryFile;
class BinaryInstallerHelper : public QObject
{
    Q_OBJECT
public:
    explicit BinaryInstallerHelper( QTemporaryFile* tempFile, const QString& resolverId, bool createAccount, AtticaManager* manager );

    virtual ~BinaryInstallerHelper();

public slots:
    void installSucceeded( const QString& path );
    void installFailed();

private:
    QTemporaryFile* m_tempFile;
    QString m_resolverId;
    bool m_createAccount;
    QPointer<AtticaManager> m_manager;
};

#endif