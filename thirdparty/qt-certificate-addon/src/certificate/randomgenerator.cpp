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
#include <gnutls/crypto.h>

#include "randomgenerator.h"

QT_BEGIN_NAMESPACE_CERTIFICATE

/*!
  \class RandomGenerator
  \brief The RandomGenerator class is a tool for creating hard random numbers.

  The RandomGenerator class provides a source of secure random numbers using
  the gnutls rnd API. The numbers are suitable for uses such as certificate
  serial numbers.
*/

/*!
  Generates a set of random bytes of the specified size. In order to allow
  these to be conveniently used as serial numbers, this method ensures that
  the value returned is positive (ie. that the first bit is 0). This means
  that you get one less bit of entropy than requested, but avoids
  interoperability issues.

  Note that this method will either return the number of bytes requested,
  or a null QByteArray. It will never return a smaller number.
 */
QByteArray RandomGenerator::getPositiveBytes(int size)
{
    QByteArray result(size, 0);

    int errnumber = gnutls_rnd(GNUTLS_RND_RANDOM, result.data(), size);
    if (GNUTLS_E_SUCCESS != errnumber)
        return QByteArray();

    // Clear the top bit to ensure the number is positive
    char *data = result.data();
    *data = *data & 0x07f;

    return result;
}

QT_END_NAMESPACE_CERTIFICATE
