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

#include <QDateTime>
#include <QSslKey>

extern "C" {
#include <gnutls/abstract.h>
};

#include "certificaterequest_p.h"
#include "utils_p.h"

#include "certificatebuilder_p.h"

QT_BEGIN_NAMESPACE_CERTIFICATE

/*!
  \class CertificateBuilder
  \brief The CertificateBuilder class is a tool for creating X.509 certificates.
*/

/*!
  Creates a new CertificateBuilder.
 */
CertificateBuilder::CertificateBuilder()
    : d(new CertificateBuilderPrivate)
{
    ensure_gnutls_init();
    d->errnumber = gnutls_x509_crt_init(&d->crt);
}

/*!
  Cleans up a CertificateBuilder.
 */
CertificateBuilder::~CertificateBuilder()
{
    gnutls_x509_crt_deinit(d->crt);
    delete d;
}

/*!
  Returns the last error that occurred when using this object. The values
  used are those of gnutls. If there has not been an error then it is
  guaranteed to be 0.
 */
int CertificateBuilder::error() const
{
    return d->errnumber;
}

/*!
  Returns a string describing the last error that occurred when using
  this object.
 */
QString CertificateBuilder::errorString() const
{
    return QString::fromUtf8(gnutls_strerror(d->errnumber));
}

/*!
  Set the request that the certificate will be generated from.
 */
bool CertificateBuilder::setRequest(const CertificateRequest &crq)
{
    d->errnumber = gnutls_x509_crt_set_crq(d->crt, crq.d->crq);
    return GNUTLS_E_SUCCESS == d->errnumber;
}

/*!
  Set the version of the X.509 certificate. In general the version will be 3.
 */
bool CertificateBuilder::setVersion(int version)
{
    d->errnumber = gnutls_x509_crt_set_version(d->crt, version);
    return GNUTLS_E_SUCCESS == d->errnumber;
}

/*!
  Set the serial number of the certificate. This should be a random value
  containing a large amount of entropy.
 */
bool CertificateBuilder::setSerial(const QByteArray &serial)
{
    d->errnumber = gnutls_x509_crt_set_serial(d->crt, serial.constData(), serial.size());
    return GNUTLS_E_SUCCESS == d->errnumber;
}

/*!
  Set the time at which the certificate will become valid.
 */
bool CertificateBuilder::setActivationTime(const QDateTime &date)
{
    d->errnumber = gnutls_x509_crt_set_activation_time(d->crt, date.toTime_t());
    return GNUTLS_E_SUCCESS == d->errnumber;
}

/*!
  Set the time after which the certificate is no longer valid.
 */
bool CertificateBuilder::setExpirationTime(const QDateTime &date)
{
    d->errnumber = gnutls_x509_crt_set_expiration_time(d->crt, date.toTime_t());
    return GNUTLS_E_SUCCESS == d->errnumber;
}

/*!
  Copies the extensions from the request to the certificate being created. This
  should only be done after checking that the request is safe, since otherwise
  you could potentially copy extensions that grant the generated certificate
  facilities you did not intend.
 */
bool CertificateBuilder::copyRequestExtensions(const CertificateRequest &crq)
{
    d->errnumber = gnutls_x509_crt_set_crq_extensions(d->crt, crq.d->crq);
    return GNUTLS_E_SUCCESS == d->errnumber;
}

/*!
  Add the basic constraints extension. This allows you to specify if the
  certificate being created is a CA (ie. may sign certificates), and the
  maximum length of the chain that is allowed if you grant it that
  permission. By default the pathLength is unlimited.
 */
bool CertificateBuilder::setBasicConstraints(bool ca, int pathLength)
{
    d->errnumber = gnutls_x509_crt_set_basic_constraints (d->crt, ca, pathLength);
    return GNUTLS_E_SUCCESS == d->errnumber;
}

/*!
  Adds the specified purpose to the list of those this certificate may be
  used for. This method may be called multiple times to add a series of
  different purposes.
 */
bool CertificateBuilder::addKeyPurpose(KeyPurpose purpose, bool critical)
{
    QByteArray ba;

    switch(purpose) {
    case PurposeWebServer:
        ba = QByteArray(GNUTLS_KP_TLS_WWW_SERVER);
        break;
    case PurposeWebClient:
        ba = QByteArray(GNUTLS_KP_TLS_WWW_CLIENT);
        break;
    case PurposeCodeSigning:
        ba = QByteArray(GNUTLS_KP_CODE_SIGNING);
        break;
    case PurposeEmailProtection:
        ba = QByteArray(GNUTLS_KP_EMAIL_PROTECTION);
        break;
    case PurposeTimeStamping:
        ba = QByteArray(GNUTLS_KP_TIME_STAMPING);
        break;
    case PurposeOcspSigning:
        ba = QByteArray(GNUTLS_KP_OCSP_SIGNING);
        break;
    case PurposeIpsecIke:
        ba = QByteArray(GNUTLS_KP_IPSEC_IKE);
        break;
    case PurposeAny:
        ba = QByteArray(GNUTLS_KP_ANY);
        break;
    default:
        qWarning("Unknown Purpose %d", int(purpose));
        return false;
    }

    return addKeyPurpose(ba, critical);
}

/*!
  Adds the specified purpose to the list of those this certificate may be
  used for. This method may be called multiple times to add a series of
  different purposes. This method differs from the one above by allowing
  arbitrary OIDs   to be used, not just those for which there is built in
  support.
 */
bool CertificateBuilder::addKeyPurpose(const QByteArray &oid, bool critical)
{
    d->errnumber = gnutls_x509_crt_set_key_purpose_oid(d->crt, oid.constData(), critical);
    return GNUTLS_E_SUCCESS == d->errnumber;
}

/*!
  Sets the key usage flags for the certificate. If you call this method more
  than once then only the last value will be used by the created certificate.
 */
bool CertificateBuilder::setKeyUsage(KeyUsageFlags usages)
{
    uint usage = 0;
    if (usages & UsageEncipherOnly)
        usage |= GNUTLS_KEY_ENCIPHER_ONLY;
    if (usages & UsageCrlSign)
        usage |= GNUTLS_KEY_CRL_SIGN;
    if (usages & UsageKeyCertSign)
        usage |= GNUTLS_KEY_KEY_CERT_SIGN;
    if (usages & UsageKeyAgreement)
        usage |= GNUTLS_KEY_KEY_AGREEMENT;
    if (usages & UsageDataEncipherment)
        usage |= GNUTLS_KEY_DATA_ENCIPHERMENT;
    if (usages & UsageKeyEncipherment)
        usage |= GNUTLS_KEY_KEY_ENCIPHERMENT;
    if (usages & UsageNonRepudiation)
        usage |= GNUTLS_KEY_NON_REPUDIATION;
    if (usages & UsageDigitalSignature)
        usage |= GNUTLS_KEY_DIGITAL_SIGNATURE;
    if (usages & UsageDecipherOnly)
        usage |= GNUTLS_KEY_DECIPHER_ONLY;

    d->errnumber = gnutls_x509_crt_set_key_usage(d->crt, usage);
    return GNUTLS_E_SUCCESS == d->errnumber;
}

/*!
  Adds the subject key identifier extension to the certificate. The key
  is extracted automatically from the certificate being created.
 */
bool CertificateBuilder::addSubjectKeyIdentifier()
{
    QByteArray ba(128, 0); // Normally 20 bytes (SHA1)
    size_t size = ba.size();

    d->errnumber = gnutls_x509_crt_get_key_id(d->crt, 0, reinterpret_cast<unsigned char *>(ba.data()), &size);
    if (GNUTLS_E_SUCCESS != d->errnumber)
        return false;

    d->errnumber = gnutls_x509_crt_set_subject_key_id (d->crt, ba.constData(), size);
    return GNUTLS_E_SUCCESS == d->errnumber;
}

/*!
  Adds the authority key identifier extension to the certificate. The key
  is extracted the specified certificate which must be the one later used
  to sign the certificate.
 */
bool CertificateBuilder::addAuthorityKeyIdentifier(const QSslCertificate &qcacert)
{
    gnutls_x509_crt_t cacrt = qsslcert_to_crt(qcacert, &d->errnumber);
    if (GNUTLS_E_SUCCESS != d->errnumber)
        return false;

    QByteArray ba(128, 0); // Normally 20 bytes (SHA1)
    size_t size = ba.size();

    // Try using the subject keyid
    d->errnumber = gnutls_x509_crt_get_subject_key_id(cacrt, reinterpret_cast<unsigned char *>(ba.data()), &size, NULL);

    // Or fallback to creating it
    if (GNUTLS_E_SUCCESS != d->errnumber) {
        d->errnumber = gnutls_x509_crt_get_key_id(cacrt, 0, reinterpret_cast<unsigned char *>(ba.data()), &size);

        if (GNUTLS_E_SUCCESS != d->errnumber) {
            gnutls_x509_crt_deinit(cacrt);
            return false;
        }
    }

    gnutls_x509_crt_deinit(cacrt);
    d->errnumber = gnutls_x509_crt_set_authority_key_id(d->crt, reinterpret_cast<const unsigned char *>(ba.constData()), size);

    return GNUTLS_E_SUCCESS == d->errnumber;
}

/*!
  Creates a self-signed certificate by signing the certificate with the specified
  key.
 */
QSslCertificate CertificateBuilder::signedCertificate(const QSslKey &qkey)
{
    gnutls_x509_privkey_t key = qsslkey_to_key(qkey, &d->errnumber);
    if (GNUTLS_E_SUCCESS != d->errnumber) {
        gnutls_x509_privkey_deinit(key);
        return QSslCertificate();
    };

    gnutls_privkey_t abstractKey;
    d->errnumber = gnutls_privkey_init(&abstractKey);
    if (GNUTLS_E_SUCCESS != d->errnumber) {
        gnutls_x509_privkey_deinit(key);
        return QSslCertificate();
    }

    gnutls_privkey_import_x509(abstractKey, key, GNUTLS_PRIVKEY_IMPORT_AUTO_RELEASE);

    d->errnumber = gnutls_x509_crt_privkey_sign(d->crt, d->crt, abstractKey, GNUTLS_DIG_SHA1, 0);

    gnutls_x509_privkey_deinit(key);

    if (GNUTLS_E_SUCCESS != d->errnumber)
        return QSslCertificate();

    return crt_to_qsslcert(d->crt, &d->errnumber);    
}

/*!
  Creates a certificate signed by the specified CA certificate using the
  CA key.
 */
QSslCertificate CertificateBuilder::signedCertificate(const QSslCertificate &qcacert, const QSslKey &qcakey)
{
    //
    // Extract the CA key
    //
    gnutls_x509_privkey_t key = qsslkey_to_key(qcakey, &d->errnumber);
    if (GNUTLS_E_SUCCESS != d->errnumber) {
        gnutls_x509_privkey_deinit(key);
        return QSslCertificate();
    };

    gnutls_privkey_t abstractKey;
    d->errnumber = gnutls_privkey_init(&abstractKey);
    if (GNUTLS_E_SUCCESS != d->errnumber) {
        gnutls_x509_privkey_deinit(key);
        return QSslCertificate();
    }

    gnutls_privkey_import_x509(abstractKey, key, GNUTLS_PRIVKEY_IMPORT_AUTO_RELEASE);

    //
    // Extract the CA cert
    //
    gnutls_x509_crt_t cacrt = qsslcert_to_crt(qcacert, &d->errnumber);
    if (GNUTLS_E_SUCCESS != d->errnumber) {
        gnutls_x509_privkey_deinit(key);
        return QSslCertificate();
    }

    //
    // Sign the cert
    //
    d->errnumber = gnutls_x509_crt_privkey_sign(d->crt, cacrt, abstractKey, GNUTLS_DIG_SHA1, 0);

    gnutls_x509_crt_deinit(cacrt);
    gnutls_x509_privkey_deinit(key);

    if (GNUTLS_E_SUCCESS != d->errnumber)
        return QSslCertificate();

    return crt_to_qsslcert(d->crt, &d->errnumber);
}

QT_END_NAMESPACE_CERTIFICATE
