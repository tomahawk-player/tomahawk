/****************************************************************************
**
** Copyright (C) 2012-2013 Richard J. Moore <rich@kde.org>
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CERTIFICATEBUILDER_H
#define CERTIFICATEBUILDER_H

#include <QString>
#include <QFlags>
#include <QtNetwork/QSslCertificate>

#include "certificate_global.h"

class QDateTime;

QT_BEGIN_NAMESPACE_CERTIFICATE

class CertificateRequest;

class Q_CERTIFICATE_EXPORT CertificateBuilder
{
public:
    enum KeyPurpose {
        PurposeWebServer,
        PurposeWebClient,
        PurposeCodeSigning,
        PurposeEmailProtection,
        PurposeTimeStamping,
        PurposeOcspSigning,
        PurposeIpsecIke,
        PurposeAny
    };

    enum KeyUsageFlag {
        UsageEncipherOnly = 0x1,
        UsageCrlSign = 0x2,
        UsageKeyCertSign = 0x4,
        UsageKeyAgreement = 0x8,
        UsageDataEncipherment = 0x10,
        UsageKeyEncipherment = 0x20,
        UsageNonRepudiation = 0x40,
        UsageDigitalSignature = 0x80,
        UsageDecipherOnly = 0x100
    };
    Q_DECLARE_FLAGS(KeyUsageFlags, KeyUsageFlag)

    CertificateBuilder();
    ~CertificateBuilder();

    int error() const;
    QString errorString() const;

    bool setRequest(const CertificateRequest &crq);

    bool setVersion(int version=3);
    bool setSerial(const QByteArray &serial);

    bool setActivationTime(const QDateTime &date);
    bool setExpirationTime(const QDateTime &date);

    // Extensions

    bool copyRequestExtensions(const CertificateRequest &crq);
    bool setBasicConstraints(bool ca=false, int pathLength=-1);

    // Extended usage
    bool addKeyPurpose(KeyPurpose purpose, bool critical=false);
    bool addKeyPurpose(const QByteArray &oid, bool critical=false);

    // Usage
    bool setKeyUsage(KeyUsageFlags usage);

    // Key identifiers
    bool addSubjectKeyIdentifier();
    bool addAuthorityKeyIdentifier(const QSslCertificate &cacert);

    QSslCertificate signedCertificate(const QSslKey &key);
    QSslCertificate signedCertificate(const QSslCertificate &cacert, const QSslKey &cakey);

private:
    struct CertificateBuilderPrivate *d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(CertificateBuilder::KeyUsageFlags)

QT_END_NAMESPACE_CERTIFICATE

#endif // CERTIFICATEBUILDER_H
