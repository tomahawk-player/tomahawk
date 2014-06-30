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

#ifndef UTILS_P_H
#define UTILS_P_H

#include <gnutls/x509.h>

#include <QtNetwork/QSsl>
#include <QtCore/QByteArray>

#include "certificate_global.h"
#include "certificate.h"

class QSslKey;
class QSslCertificate;

QT_BEGIN_NAMESPACE_CERTIFICATE

void ensure_gnutls_init();

QByteArray entrytype_to_oid(Certificate::EntryType type);

gnutls_x509_privkey_t qsslkey_to_key(const QSslKey &qkey, int *errnumber);
gnutls_x509_crt_t qsslcert_to_crt(const QSslCertificate &qcert, int *errnumber);

QSslCertificate crt_to_qsslcert(gnutls_x509_crt_t crt, int *errnumber);
QSslKey key_to_qsslkey(gnutls_x509_privkey_t key, QSsl::KeyAlgorithm algo, int *errnumber);

#if QT_VERSION >= 0x050000
gnutls_x509_subject_alt_name_t qssl_altnameentrytype_to_altname(QSsl::AlternativeNameEntryType qtype);
#else
gnutls_x509_subject_alt_name_t qssl_altnameentrytype_to_altname(QSsl::AlternateNameEntryType qtype);
#endif

QT_END_NAMESPACE_CERTIFICATE

#endif // UTILS_P_H
