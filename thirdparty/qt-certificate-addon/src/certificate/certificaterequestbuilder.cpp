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

#include <QIODevice>

#include "certificaterequest_p.h"
#include "utils_p.h"

#include "certificaterequestbuilder_p.h"

QT_BEGIN_NAMESPACE_CERTIFICATE

/*!
  \class CertificateRequestBuilder
  \brief The CertificateRequestBuilder class is a tool for creating certificate
  signing requests.
*/

/*!
  Create a new CertificateRequestBuilder.
*/
CertificateRequestBuilder::CertificateRequestBuilder()
    : d(new CertificateRequestBuilderPrivate)
{
    ensure_gnutls_init();

    gnutls_x509_crq_init(&d->crq);
    d->errnumber = GNUTLS_E_SUCCESS;
}

/*!
  Cleans up a CertificateRequestBuilder.
*/
CertificateRequestBuilder::~CertificateRequestBuilder()
{
    gnutls_x509_crq_deinit(d->crq);
    delete d;
}

/*!
  Returns the last error that occurred when using this object. The values
  used are those of gnutls. If there has not been an error then it is
  guaranteed to be 0.
 */
int CertificateRequestBuilder::error() const
{
    return d->errnumber;
}

/*!
  Returns a string describing the last error that occurred when using
  this object.
 */
QString CertificateRequestBuilder::errorString() const
{
    return QString::fromUtf8(gnutls_strerror(d->errnumber));
}

/*!
  Set the version of the certificate signing request. This should
  generally be set to 1.
 */
bool CertificateRequestBuilder::setVersion(int version)
{
    d->errnumber = gnutls_x509_crq_set_version(d->crq, version);
    return GNUTLS_E_SUCCESS == d->errnumber;
}

/*!
  Returns the version of the certificate signing request.
 */
int CertificateRequestBuilder::version() const
{
    int ver = gnutls_x509_crq_get_version(d->crq);
    if (ver < 0)
        d->errnumber = ver;
    return ver;
}

/*!
  Sets the key that will be used for the reqest.
 */
bool CertificateRequestBuilder::setKey(const QSslKey &qkey)
{
    gnutls_x509_privkey_t key = qsslkey_to_key(qkey, &d->errnumber);
    if (GNUTLS_E_SUCCESS != d->errnumber) {
        gnutls_x509_privkey_deinit(key);
        return false;
    };

    d->errnumber = gnutls_x509_crq_set_key(d->crq, key);

    gnutls_x509_privkey_deinit(key);

    return GNUTLS_E_SUCCESS == d->errnumber;
}

/*!
  Returns the list of attributes that are present in this requests
  distinguished name. The attributes are returned as OIDs.
 */
QList<QByteArray> CertificateRequestBuilder::nameEntryAttributes()
{
    QList<QByteArray> result;

    int index = 0;
    do {
        QByteArray buffer(1024, 0);
        size_t size = buffer.size();

        d->errnumber = gnutls_x509_crq_get_dn_oid(d->crq, index, buffer.data(), &size);

        if (GNUTLS_E_SUCCESS == d->errnumber) {
            buffer.resize(size);
            result << buffer;
        }
        index++;
    } while(GNUTLS_E_SUCCESS == d->errnumber);

    return result;
}

/*!
  Returns the list of entries for the attribute specified.
 */
QStringList CertificateRequestBuilder::nameEntryInfo(Certificate::EntryType attribute)
{
    return nameEntryInfo(entrytype_to_oid(attribute));
}

/*!
  Returns the list of entries for the attribute specified by the oid.
 */
QStringList CertificateRequestBuilder::nameEntryInfo(const QByteArray &oid)
{
    QStringList result;
    if (oid.isNull())
        return result;

    int index = 0;
    do {
        QByteArray buffer(1024, 0);
        size_t size = buffer.size();

        d->errnumber = gnutls_x509_crq_get_dn_by_oid(d->crq, oid.constData(), index, false, buffer.data(), &size);

        if (GNUTLS_E_SUCCESS == d->errnumber)
            result << QString::fromUtf8(buffer);

        index++;
    } while(GNUTLS_E_SUCCESS == d->errnumber);

    return result;
}

/*!
  Adds an entry to the distinguished name of the certificate signing request. The
  \a type parameter specifies the field that will be added, and the \a value
  parameter specifies the value. This method can be called multiple times with the
  same \a type and will add additional entries.
 */
bool CertificateRequestBuilder::addNameEntry(Certificate::EntryType type, const QByteArray &value)
{
    QByteArray oid = entrytype_to_oid(type);
    if (oid.isNull())
        return false;

    return addNameEntry(oid, value);
}

/*!
  Adds an entry to the distinguished name of the certificate signing request. The
  \a oid parameter specifies the ASN.1 OID of the field that will be added, and
  the \a value parameter specifies the value. If the \a raw flag is set to true
  then OIDs not understood by GNUTLS may be added, and the value must be DER
  encoded.  This method can be called multiple times with the same \a type and
  will add additional entries.
 */
bool CertificateRequestBuilder::addNameEntry(const QByteArray &oid, const QByteArray &value, bool raw)
{
    d->errnumber = gnutls_x509_crq_set_dn_by_oid(d->crq, oid.constData(), raw,
                                             value.constData(), qstrlen(value.constData()));
    return GNUTLS_E_SUCCESS == d->errnumber;
}

#if QT_VERSION >= 0x050000
bool CertificateRequestBuilder::addSubjectAlternativeNameEntry(QSsl::AlternativeNameEntryType qtype, const QByteArray &value)
{
    gnutls_x509_subject_alt_name_t type = qssl_altnameentrytype_to_altname(qtype);

    d->errnumber = gnutls_x509_crq_set_subject_alt_name(d->crq, type, value.constData(), value.size(), GNUTLS_FSAN_APPEND);
    return GNUTLS_E_SUCCESS == d->errnumber;
}
#else
bool CertificateRequestBuilder::addSubjectAlternativeNameEntry(QSsl::AlternateNameEntryType qtype, const QByteArray &value)
{
    gnutls_x509_subject_alt_name_t type = qssl_altnameentrytype_to_altname(qtype);

    d->errnumber = gnutls_x509_crq_set_subject_alt_name(d->crq, type, value.constData(), value.size(), GNUTLS_FSAN_APPEND);
    return GNUTLS_E_SUCCESS == d->errnumber;
}
#endif

/*!
  Signs the request with the specified key and returns the signed request.
 */
CertificateRequest CertificateRequestBuilder::signedRequest(const QSslKey &qkey)
{
    CertificateRequest result;

    gnutls_x509_privkey_t key = qsslkey_to_key(qkey, &d->errnumber);
    if (GNUTLS_E_SUCCESS != d->errnumber) {
        gnutls_x509_privkey_deinit(key);
        return result;
    };

    d->errnumber = gnutls_x509_crq_sign2(d->crq, key, GNUTLS_DIG_SHA1, 0);
    gnutls_x509_privkey_deinit(key);

    if (GNUTLS_E_SUCCESS != d->errnumber)
        return result;

    gnutls_x509_crq_t crqsave = result.d->crq;
    result.d->crq = d->crq;
    d->crq = crqsave;

    return result;
}

QT_END_NAMESPACE_CERTIFICATE
