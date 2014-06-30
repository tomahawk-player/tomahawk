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

#include <QByteArray>
#include <QIODevice>
#include <QStringList>
#include <QDebug>

#include "utils_p.h"

#include "certificaterequest_p.h"

QT_BEGIN_NAMESPACE_CERTIFICATE

/*!
  \class CertificateRequest
  \brief The CertificateRequest class provides a convenient interface for an X.509
  certificate signing request.
*/

/*!
  \internal
  Convert a crq to a PEM or DER encoded QByteArray.
 */
static QByteArray request_to_bytearray(gnutls_x509_crq_t crq, gnutls_x509_crt_fmt_t format, int *errnumber)
{
    QByteArray ba(4096, 0);
    size_t size = ba.size();

    *errnumber = gnutls_x509_crq_export(crq, format, ba.data(), &size);

    if (GNUTLS_E_SUCCESS != *errnumber)
        return QByteArray();

    ba.resize(size); // size has now been updated
    return ba;
}

CertificateRequestPrivate::CertificateRequestPrivate()
    : null(true)
{
    ensure_gnutls_init();

    gnutls_x509_crq_init(&crq);
    errnumber = GNUTLS_E_SUCCESS;
}

CertificateRequestPrivate::~CertificateRequestPrivate()
{
    gnutls_x509_crq_deinit(crq);
}

/*!
  Create a null CertificateRequest.
 */
CertificateRequest::CertificateRequest()
    : d(new CertificateRequestPrivate)
{
}

/*!
  Create a CertificateRequest that is a copy of other.
 */
CertificateRequest::CertificateRequest(const CertificateRequest &other)
    : d(other.d)
{
}

/*!
  Load a CertificateRequest from the specified QIODevice using the specified format.
 */
CertificateRequest::CertificateRequest(QIODevice *io, QSsl::EncodingFormat format)
    : d(new CertificateRequestPrivate)
{
    QByteArray buf = io->readAll();

    // Setup a datum
    gnutls_datum_t buffer;
    buffer.data = (unsigned char *)(buf.data());
    buffer.size = buf.size();

    d->errnumber = gnutls_x509_crq_import(d->crq, &buffer, (QSsl::Pem == format) ? GNUTLS_X509_FMT_PEM : GNUTLS_X509_FMT_DER);
    if (GNUTLS_E_SUCCESS == d->errnumber)
        d->null = false;
}

/*!
  Clean up.
 */
CertificateRequest::~CertificateRequest()
{
}

/*!
  Returns true if this CertificateRequest is null (uninitialised).
 */
bool CertificateRequest::isNull() const
{
    return d->null;
}

/*!
  Returns the last error that occurred when using this object. The values
  used are those of gnutls. If there has not been an error then it is
  guaranteed to be 0.
 */
int CertificateRequest::error() const
{
    return d->errnumber;
}

/*!
  Returns a string describing the last error that occurred when using
  this object.
 */
QString CertificateRequest::errorString() const
{
    return QString::fromUtf8(gnutls_strerror(d->errnumber));
}

/*!
  Returns the version of the certificate signing request.
 */
int CertificateRequest::version() const
{
    return gnutls_x509_crq_get_version(d->crq);
}

/*!
  Returns the list of attributes that are present in this requests
  distinguished name. The attributes are returned as OIDs.
 */
QList<QByteArray> CertificateRequest::nameEntryAttributes()
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
QStringList CertificateRequest::nameEntryInfo(Certificate::EntryType attribute)
{
    return nameEntryInfo(entrytype_to_oid(attribute));
}

/*!
  Returns the list of entries for the attribute specified by the oid.
 */
QStringList CertificateRequest::nameEntryInfo(const QByteArray &oid)
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
  Returns a QByteArray containing this request encoded as PEM.
 */
QByteArray CertificateRequest::toPem()
{
    return request_to_bytearray(d->crq, GNUTLS_X509_FMT_PEM, &d->errnumber);
}

/*!
  Returns a QByteArray containing this request encoded as DER.
 */
QByteArray CertificateRequest::toDer()
{
    return request_to_bytearray(d->crq, GNUTLS_X509_FMT_DER, &d->errnumber);
}

/*!
  Returns a QString containing this request as a human readable string.
 */
QString CertificateRequest::toText()
{
    gnutls_datum_t datum;
    d->errnumber = gnutls_x509_crq_print(d->crq, GNUTLS_CRT_PRINT_FULL, &datum);

    if (GNUTLS_E_SUCCESS != d->errnumber)
        return QString();

    QString result = QString::fromUtf8(reinterpret_cast<const char *>(datum.data), datum.size);
    gnutls_free(datum.data);

    return result;
}

QT_END_NAMESPACE_CERTIFICATE
