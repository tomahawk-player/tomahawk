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

#include <gnutls/gnutls.h>

#include <QByteArray>
#include <QSslKey>
#include <QSslCertificate>

#include "utils_p.h"

QT_BEGIN_NAMESPACE_CERTIFICATE

using namespace Certificate;

void ensure_gnutls_init()
{
    static bool done = false;

    // TODO: protect with a mutex
    if (!done) {
        gnutls_global_init();
        done = true;
    }
}

QByteArray entrytype_to_oid(Certificate::EntryType type)
{
    QByteArray oid;

    // TODO: More common name entry types

    switch(type) {
    case EntryCountryName:
        oid = QByteArray(GNUTLS_OID_X520_COUNTRY_NAME);
        break;
    case EntryOrganizationName:
        oid = QByteArray(GNUTLS_OID_X520_ORGANIZATION_NAME);
        break;
    case EntryOrganizationalUnitName:
        oid = QByteArray(GNUTLS_OID_X520_ORGANIZATIONAL_UNIT_NAME);
        break;
    case EntryCommonName:
        oid = QByteArray(GNUTLS_OID_X520_COMMON_NAME);
        break;
    case EntryLocalityName:
        oid = QByteArray(GNUTLS_OID_X520_LOCALITY_NAME);
        break;
    case EntryStateOrProvinceName:
        oid = QByteArray(GNUTLS_OID_X520_STATE_OR_PROVINCE_NAME);
        break;
    case EntryEmail:
        oid = QByteArray(GNUTLS_OID_PKCS9_EMAIL);
        break;
    default:
        qWarning("Unhandled name entry type %d", int(type));
    }

    return oid;
}

gnutls_x509_privkey_t qsslkey_to_key(const QSslKey &qkey, int *errnumber)
{
    gnutls_x509_privkey_t key;

    *errnumber = gnutls_x509_privkey_init(&key);
    if (GNUTLS_E_SUCCESS != *errnumber)
        return 0;

    QByteArray buf(qkey.toPem());

    // Setup a datum
    gnutls_datum_t buffer;
    buffer.data = (unsigned char *)(buf.data());
    buffer.size = buf.size();

    *errnumber = gnutls_x509_privkey_import(key, &buffer, GNUTLS_X509_FMT_PEM);
    return key;
}

gnutls_x509_crt_t qsslcert_to_crt(const QSslCertificate &qcert, int *errnumber)
{
    gnutls_x509_crt_t cert;

    *errnumber = gnutls_x509_crt_init(&cert);
    if (GNUTLS_E_SUCCESS != *errnumber)
        return 0;

    QByteArray buf(qcert.toPem());

    // Setup a datum
    gnutls_datum_t buffer;
    buffer.data = (unsigned char *)(buf.data());
    buffer.size = buf.size();

    // Import the cert
    *errnumber = gnutls_x509_crt_import(cert, &buffer, GNUTLS_X509_FMT_PEM);
    return cert;
}

QSslCertificate crt_to_qsslcert(gnutls_x509_crt_t crt, int *errnumber)
{
    QByteArray ba(4096, 0);
    size_t size = ba.size();

    *errnumber = gnutls_x509_crt_export(crt, GNUTLS_X509_FMT_PEM, ba.data(), &size);
    if (GNUTLS_E_SUCCESS != *errnumber)
        return QSslCertificate();

    return QSslCertificate(ba);
}

QSslKey key_to_qsslkey(gnutls_x509_privkey_t key, QSsl::KeyAlgorithm algo, int *errnumber)
{
    QByteArray ba(4096, 0);
    size_t size = ba.size();

    *errnumber = gnutls_x509_privkey_export(key, GNUTLS_X509_FMT_PEM, ba.data(), &size);
    if (GNUTLS_E_SUCCESS != *errnumber)
        return QSslKey();

    return QSslKey(ba, algo);
}

#if QT_VERSION >= 0x050000
gnutls_x509_subject_alt_name_t qssl_altnameentrytype_to_altname(QSsl::AlternativeNameEntryType qtype)
{
    switch(qtype) {
    case QSsl::EmailEntry:
        return GNUTLS_SAN_RFC822NAME;
    case QSsl::DnsEntry:
        return GNUTLS_SAN_DNSNAME;
    default:
        qWarning("Unknown alternative name type %d", int(qtype));
    }
    return GNUTLS_SAN_OTHERNAME;
}
#else
gnutls_x509_subject_alt_name_t qssl_altnameentrytype_to_altname(QSsl::AlternateNameEntryType qtype)
{
    switch(qtype) {
    case QSsl::EmailEntry:
        return GNUTLS_SAN_RFC822NAME;
    case QSsl::DnsEntry:
        return GNUTLS_SAN_DNSNAME;
    default:
        qWarning("Unknown alternative name type %d", int(qtype));
    }

    return GNUTLS_SAN_OTHERNAME;
}
#endif

QT_END_NAMESPACE_CERTIFICATE
