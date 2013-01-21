/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

//#define QHTTP_DEBUG

#include "qhttpheader.h"
#include <QtCore/QSet>


class QHttpHeaderPrivate
{
    Q_DECLARE_PUBLIC(QHttpHeader)
public:
    inline virtual ~QHttpHeaderPrivate() {}

    QList<QPair<QString, QString> > values;
    bool valid;
    QHttpHeader *q_ptr;
};

/****************************************************
 *
 * QHttpHeader
 *
 ****************************************************/

/*!
    \class QHttpHeader
    \obsolete
    \brief The QHttpHeader class contains header information for HTTP.

    \ingroup network
    \inmodule QtNetwork

    In most cases you should use the more specialized derivatives of
    this class, QHttpResponseHeader and QHttpRequestHeader, rather
    than directly using QHttpHeader.

    QHttpHeader provides the HTTP header fields. A HTTP header field
    consists of a name followed by a colon, a single space, and the
    field value. (See RFC 1945.) Field names are case-insensitive. A
    typical header field looks like this:
    \snippet doc/src/snippets/code/src_network_access_qhttp.cpp 0

    In the API the header field name is called the "key" and the
    content is called the "value". You can get and set a header
    field's value by using its key with value() and setValue(), e.g.
    \snippet doc/src/snippets/code/src_network_access_qhttp.cpp 1

    Some fields are so common that getters and setters are provided
    for them as a convenient alternative to using \l value() and
    \l setValue(), e.g. contentLength() and contentType(),
    setContentLength() and setContentType().

    Each header key has a \e single value associated with it. If you
    set the value for a key which already exists the previous value
    will be discarded.

    \sa QHttpRequestHeader QHttpResponseHeader
*/

/*!
    \fn int QHttpHeader::majorVersion() const

    Returns the major protocol-version of the HTTP header.
*/

/*!
    \fn int QHttpHeader::minorVersion() const

    Returns the minor protocol-version of the HTTP header.
*/

/*!
        Constructs an empty HTTP header.
*/
QHttpHeader::QHttpHeader()
    : d_ptr(new QHttpHeaderPrivate)
{
    Q_D(QHttpHeader);
    d->q_ptr = this;
    d->valid = true;
}

/*!
        Constructs a copy of \a header.
*/
QHttpHeader::QHttpHeader(const QHttpHeader &header)
    : d_ptr(new QHttpHeaderPrivate)
{
    Q_D(QHttpHeader);
    d->q_ptr = this;
    d->valid = header.d_func()->valid;
    d->values = header.d_func()->values;
}

/*!
    Constructs a HTTP header for \a str.

    This constructor parses the string \a str for header fields and
    adds this information. The \a str should consist of one or more
    "\r\n" delimited lines; each of these lines should have the format
    key, colon, space, value.
*/
QHttpHeader::QHttpHeader(const QString &str)
    : d_ptr(new QHttpHeaderPrivate)
{
    Q_D(QHttpHeader);
    d->q_ptr = this;
    d->valid = true;
    parse(str);
}

/*! \internal
 */
QHttpHeader::QHttpHeader(QHttpHeaderPrivate &dd, const QString &str)
    : d_ptr(&dd)
{
    Q_D(QHttpHeader);
    d->q_ptr = this;
    d->valid = true;
    if (!str.isEmpty())
        parse(str);
}

/*! \internal
 */
QHttpHeader::QHttpHeader(QHttpHeaderPrivate &dd, const QHttpHeader &header)
    : d_ptr(&dd)
{
    Q_D(QHttpHeader);
    d->q_ptr = this;
    d->valid = header.d_func()->valid;
    d->values = header.d_func()->values;
}
/*!
    Destructor.
*/
QHttpHeader::~QHttpHeader()
{
}

/*!
    Assigns \a h and returns a reference to this http header.
*/
QHttpHeader &QHttpHeader::operator=(const QHttpHeader &h)
{
    Q_D(QHttpHeader);
    d->values = h.d_func()->values;
    d->valid = h.d_func()->valid;
    return *this;
}

/*!
    Returns true if the HTTP header is valid; otherwise returns false.

    A QHttpHeader is invalid if it was created by parsing a malformed string.
*/
bool QHttpHeader::isValid() const
{
    Q_D(const QHttpHeader);
    return d->valid;
}

/*! \internal
    Parses the HTTP header string \a str for header fields and adds
    the keys/values it finds. If the string is not parsed successfully
    the QHttpHeader becomes \link isValid() invalid\endlink.

    Returns true if \a str was successfully parsed; otherwise returns false.

    \sa toString()
*/
bool QHttpHeader::parse(const QString &str)
{
    Q_D(QHttpHeader);
    QStringList lst;
    int pos = str.indexOf(QLatin1Char('\n'));
    if (pos > 0 && str.at(pos - 1) == QLatin1Char('\r'))
        lst = str.trimmed().split(QLatin1String("\r\n"));
    else
        lst = str.trimmed().split(QLatin1String("\n"));
    lst.removeAll(QString()); // No empties

    if (lst.isEmpty())
        return true;

    QStringList lines;
    QStringList::Iterator it = lst.begin();
    for (; it != lst.end(); ++it) {
        if (!(*it).isEmpty()) {
            if ((*it)[0].isSpace()) {
                if (!lines.isEmpty()) {
                    lines.last() += QLatin1Char(' ');
                    lines.last() += (*it).trimmed();
                }
            } else {
                lines.append((*it));
            }
        }
    }

    int number = 0;
    it = lines.begin();
    for (; it != lines.end(); ++it) {
        if (!parseLine(*it, number++)) {
            d->valid = false;
            return false;
        }
    }
    return true;
}

/*! \internal
*/
void QHttpHeader::setValid(bool v)
{
    Q_D(QHttpHeader);
    d->valid = v;
}

/*!
    Returns the first value for the entry with the given \a key. If no entry
    has this \a key, an empty string is returned.

    \sa setValue() removeValue() hasKey() keys()
*/
QString QHttpHeader::value(const QString &key) const
{
    Q_D(const QHttpHeader);
    QString lowercaseKey = key.toLower();
    QList<QPair<QString, QString> >::ConstIterator it = d->values.constBegin();
    while (it != d->values.constEnd()) {
        if ((*it).first.toLower() == lowercaseKey)
            return (*it).second;
        ++it;
    }
    return QString();
}

/*!
    Returns all the entries with the given \a key. If no entry
    has this \a key, an empty string list is returned.
*/
QStringList QHttpHeader::allValues(const QString &key) const
{
    Q_D(const QHttpHeader);
    QString lowercaseKey = key.toLower();
    QStringList valueList;
    QList<QPair<QString, QString> >::ConstIterator it = d->values.constBegin();
    while (it != d->values.constEnd()) {
        if ((*it).first.toLower() == lowercaseKey)
            valueList.append((*it).second);
        ++it;
    }
    return valueList;
}

/*!
    Returns a list of the keys in the HTTP header.

    \sa hasKey()
*/
QStringList QHttpHeader::keys() const
{
    Q_D(const QHttpHeader);
    QStringList keyList;
    QSet<QString> seenKeys;
    QList<QPair<QString, QString> >::ConstIterator it = d->values.constBegin();
    while (it != d->values.constEnd()) {
        const QString &key = (*it).first;
        QString lowercaseKey = key.toLower();
        if (!seenKeys.contains(lowercaseKey)) {
            keyList.append(key);
            seenKeys.insert(lowercaseKey);
        }
        ++it;
    }
    return keyList;
}

/*!
    Returns true if the HTTP header has an entry with the given \a
    key; otherwise returns false.

    \sa value() setValue() keys()
*/
bool QHttpHeader::hasKey(const QString &key) const
{
    Q_D(const QHttpHeader);
    QString lowercaseKey = key.toLower();
    QList<QPair<QString, QString> >::ConstIterator it = d->values.constBegin();
    while (it != d->values.constEnd()) {
        if ((*it).first.toLower() == lowercaseKey)
            return true;
        ++it;
    }
    return false;
}

/*!
    Sets the value of the entry with the \a key to \a value.

    If no entry with \a key exists, a new entry with the given \a key
    and \a value is created. If an entry with the \a key already
    exists, the first value is discarded and replaced with the given
    \a value.

    \sa value() hasKey() removeValue()
*/
void QHttpHeader::setValue(const QString &key, const QString &value)
{
    Q_D(QHttpHeader);
    QString lowercaseKey = key.toLower();
    QList<QPair<QString, QString> >::Iterator it = d->values.begin();
    while (it != d->values.end()) {
        if ((*it).first.toLower() == lowercaseKey) {
            (*it).second = value;
            return;
        }
        ++it;
    }
    // not found so add
    addValue(key, value);
}

/*!
    Sets the header entries to be the list of key value pairs in \a values.
*/
void QHttpHeader::setValues(const QList<QPair<QString, QString> > &values)
{
    Q_D(QHttpHeader);
    d->values = values;
}

/*!
    Adds a new entry with the \a key and \a value.
*/
void QHttpHeader::addValue(const QString &key, const QString &value)
{
    Q_D(QHttpHeader);
    d->values.append(qMakePair(key, value));
}

/*!
    Returns all the entries in the header.
*/
QList<QPair<QString, QString> > QHttpHeader::values() const
{
    Q_D(const QHttpHeader);
    return d->values;
}

/*!
    Removes the entry with the key \a key from the HTTP header.

    \sa value() setValue()
*/
void QHttpHeader::removeValue(const QString &key)
{
    Q_D(QHttpHeader);
    QString lowercaseKey = key.toLower();
    QList<QPair<QString, QString> >::Iterator it = d->values.begin();
    while (it != d->values.end()) {
        if ((*it).first.toLower() == lowercaseKey) {
            d->values.erase(it);
            return;
        }
        ++it;
    }
}

/*!
    Removes all the entries with the key \a key from the HTTP header.
*/
void QHttpHeader::removeAllValues(const QString &key)
{
    Q_D(QHttpHeader);
    QString lowercaseKey = key.toLower();
    QList<QPair<QString, QString> >::Iterator it = d->values.begin();
    while (it != d->values.end()) {
        if ((*it).first.toLower() == lowercaseKey) {
            it = d->values.erase(it);
            continue;
        }
        ++it;
    }
}

/*! \internal
    Parses the single HTTP header line \a line which has the format
    key, colon, space, value, and adds key/value to the headers. The
    linenumber is \a number. Returns true if the line was successfully
    parsed and the key/value added; otherwise returns false.

    \sa parse()
*/
bool QHttpHeader::parseLine(const QString &line, int)
{
    int i = line.indexOf(QLatin1Char(':'));
    if (i == -1)
        return false;

    addValue(line.left(i).trimmed(), line.mid(i + 1).trimmed());

    return true;
}

/*!
    Returns a string representation of the HTTP header.

    The string is suitable for use by the constructor that takes a
    QString. It consists of lines with the format: key, colon, space,
    value, "\r\n".
*/
QString QHttpHeader::toString() const
{
    Q_D(const QHttpHeader);
    if (!isValid())
        return QLatin1String("");

    QString ret = QLatin1String("");

    QList<QPair<QString, QString> >::ConstIterator it = d->values.constBegin();
    while (it != d->values.constEnd()) {
        ret += (*it).first + QLatin1String(": ") + (*it).second + QLatin1String("\r\n");
        ++it;
    }
    return ret;
}

/*!
    Returns true if the header has an entry for the special HTTP
    header field \c content-length; otherwise returns false.

    \sa contentLength() setContentLength()
*/
bool QHttpHeader::hasContentLength() const
{
    return hasKey(QLatin1String("content-length"));
}

/*!
    Returns the value of the special HTTP header field \c
    content-length.

    \sa setContentLength() hasContentLength()
*/
uint QHttpHeader::contentLength() const
{
    return value(QLatin1String("content-length")).toUInt();
}

/*!
    Sets the value of the special HTTP header field \c content-length
    to \a len.

    \sa contentLength() hasContentLength()
*/
void QHttpHeader::setContentLength(int len)
{
    setValue(QLatin1String("content-length"), QString::number(len));
}

/*!
    Returns true if the header has an entry for the special HTTP
    header field \c content-type; otherwise returns false.

    \sa contentType() setContentType()
*/
bool QHttpHeader::hasContentType() const
{
    return hasKey(QLatin1String("content-type"));
}

/*!
    Returns the value of the special HTTP header field \c content-type.

    \sa setContentType() hasContentType()
*/
QString QHttpHeader::contentType() const
{
    QString type = value(QLatin1String("content-type"));
    if (type.isEmpty())
        return QString();

    int pos = type.indexOf(QLatin1Char(';'));
    if (pos == -1)
        return type;

    return type.left(pos).trimmed();
}

/*!
    Sets the value of the special HTTP header field \c content-type to
    \a type.

    \sa contentType() hasContentType()
*/
void QHttpHeader::setContentType(const QString &type)
{
    setValue(QLatin1String("content-type"), type);
}

class QHttpResponseHeaderPrivate : public QHttpHeaderPrivate
{
    Q_DECLARE_PUBLIC(QHttpResponseHeader)
public:
    int statCode;
    QString reasonPhr;
    int majVer;
    int minVer;
};

/****************************************************
 *
 * QHttpResponseHeader
 *
 ****************************************************/

/*!
    \class QHttpResponseHeader
    \obsolete
    \brief The QHttpResponseHeader class contains response header information for HTTP.

    \ingroup network
    \inmodule QtNetwork

    This class is used by the QHttp class to report the header
    information that the client received from the server.

    HTTP responses have a status code that indicates the status of the
    response. This code is a 3-digit integer result code (for details
    see to RFC 1945). In addition to the status code, you can also
    specify a human-readable text that describes the reason for the
    code ("reason phrase"). This class allows you to get the status
    code and the reason phrase.

    \sa QHttpRequestHeader, QHttp, {HTTP Example}
*/

/*!
    Constructs an empty HTTP response header.
*/
QHttpResponseHeader::QHttpResponseHeader()
    : QHttpHeader(*new QHttpResponseHeaderPrivate)
{
    setValid(false);
}

/*!
    Constructs a copy of \a header.
*/
QHttpResponseHeader::QHttpResponseHeader(const QHttpResponseHeader &header)
    : QHttpHeader(*new QHttpResponseHeaderPrivate, header)
{
    Q_D(QHttpResponseHeader);
    d->statCode = header.d_func()->statCode;
    d->reasonPhr = header.d_func()->reasonPhr;
    d->majVer = header.d_func()->majVer;
    d->minVer = header.d_func()->minVer;
}

/*!
    Copies the contents of \a header into this QHttpResponseHeader.
*/
QHttpResponseHeader &QHttpResponseHeader::operator=(const QHttpResponseHeader &header)
{
    Q_D(QHttpResponseHeader);
    QHttpHeader::operator=(header);
    d->statCode = header.d_func()->statCode;
    d->reasonPhr = header.d_func()->reasonPhr;
    d->majVer = header.d_func()->majVer;
    d->minVer = header.d_func()->minVer;
    return *this;
}

/*!
    Constructs a HTTP response header from the string \a str. The
    string is parsed and the information is set. The \a str should
    consist of one or more "\r\n" delimited lines; the first line should be the
    status-line (format: HTTP-version, space, status-code, space,
    reason-phrase); each of remaining lines should have the format key, colon,
    space, value.
*/
QHttpResponseHeader::QHttpResponseHeader(const QString &str)
    : QHttpHeader(*new QHttpResponseHeaderPrivate)
{
    parse(str);
}

/*!
    \since 4.1

    Constructs a QHttpResponseHeader, setting the status code to \a code, the
    reason phrase to \a text and the protocol-version to \a majorVer and \a
    minorVer.

    \sa statusCode() reasonPhrase() majorVersion() minorVersion()
*/
QHttpResponseHeader::QHttpResponseHeader(int code, const QString &text, int majorVer, int minorVer)
    : QHttpHeader(*new QHttpResponseHeaderPrivate)
{
    setStatusLine(code, text, majorVer, minorVer);
}

/*!
    \since 4.1

    Sets the status code to \a code, the reason phrase to \a text and
    the protocol-version to \a majorVer and \a minorVer.

    \sa statusCode() reasonPhrase() majorVersion() minorVersion()
*/
void QHttpResponseHeader::setStatusLine(int code, const QString &text, int majorVer, int minorVer)
{
    Q_D(QHttpResponseHeader);
    setValid(true);
    d->statCode = code;
    d->reasonPhr = text;
    d->majVer = majorVer;
    d->minVer = minorVer;
}

/*!
    Returns the status code of the HTTP response header.

    \sa reasonPhrase() majorVersion() minorVersion()
*/
int QHttpResponseHeader::statusCode() const
{
    Q_D(const QHttpResponseHeader);
    return d->statCode;
}

/*!
    Returns the reason phrase of the HTTP response header.

    \sa statusCode() majorVersion() minorVersion()
*/
QString QHttpResponseHeader::reasonPhrase() const
{
    Q_D(const QHttpResponseHeader);
    return d->reasonPhr;
}

/*!
    Returns the major protocol-version of the HTTP response header.

    \sa minorVersion() statusCode() reasonPhrase()
*/
int QHttpResponseHeader::majorVersion() const
{
    Q_D(const QHttpResponseHeader);
    return d->majVer;
}

/*!
    Returns the minor protocol-version of the HTTP response header.

    \sa majorVersion() statusCode() reasonPhrase()
*/
int QHttpResponseHeader::minorVersion() const
{
    Q_D(const QHttpResponseHeader);
    return d->minVer;
}

/*! \internal
*/
bool QHttpResponseHeader::parseLine(const QString &line, int number)
{
    Q_D(QHttpResponseHeader);
    if (number != 0)
        return QHttpHeader::parseLine(line, number);

    QString l = line.simplified();
    if (l.length() < 10)
        return false;

    if (l.left(5) == QLatin1String("HTTP/") && l[5].isDigit() && l[6] == QLatin1Char('.') &&
        l[7].isDigit() && l[8] == QLatin1Char(' ') && l[9].isDigit()) {
        d->majVer = l[5].toLatin1() - '0';
        d->minVer = l[7].toLatin1() - '0';

        int pos = l.indexOf(QLatin1Char(' '), 9);
        if (pos != -1) {
            d->reasonPhr = l.mid(pos + 1);
            d->statCode = l.mid(9, pos - 9).toInt();
        } else {
            d->statCode = l.mid(9).toInt();
            d->reasonPhr.clear();
        }
    } else {
        return false;
    }

    return true;
}

/*! \reimp
*/
QString QHttpResponseHeader::toString() const
{
    Q_D(const QHttpResponseHeader);
    QString ret(QLatin1String("HTTP/%1.%2 %3 %4\r\n%5\r\n"));
    return ret.arg(d->majVer).arg(d->minVer).arg(d->statCode).arg(d->reasonPhr).arg(QHttpHeader::toString());
}

class QHttpRequestHeaderPrivate : public QHttpHeaderPrivate
{
    Q_DECLARE_PUBLIC(QHttpRequestHeader)
public:
    QString m;
    QString p;
    int majVer;
    int minVer;
};

/****************************************************
 *
 * QHttpRequestHeader
 *
 ****************************************************/

/*!
    \class QHttpRequestHeader
    \obsolete
    \brief The QHttpRequestHeader class contains request header information for HTTP.

    \ingroup network
    \inmodule QtNetwork

    This class is used in the QHttp class to report the header
    information if the client requests something from the server.

    HTTP requests have a method which describes the request's action.
    The most common requests are "GET" and "POST". In addition to the
    request method the header also includes a request-URI to specify
    the location for the method to use.

    The method, request-URI and protocol-version can be set using a
    constructor or later using setRequest(). The values can be
    obtained using method(), path(), majorVersion() and
    minorVersion().

    Note that the request-URI must be in the format expected by the
    HTTP server. That is, all reserved characters must be encoded in
    %HH (where HH are two hexadecimal digits). See
    QUrl::toPercentEncoding() for more information.

    Important inherited functions: setValue() and value().

    \sa QHttpResponseHeader QHttp
*/

/*!
    Constructs an empty HTTP request header.
*/
QHttpRequestHeader::QHttpRequestHeader()
    : QHttpHeader(*new QHttpRequestHeaderPrivate)
{
    setValid(false);
}

/*!
    Constructs a HTTP request header for the method \a method, the
    request-URI \a path and the protocol-version \a majorVer and \a
    minorVer. The \a path argument must be properly encoded for an
    HTTP request.
*/
QHttpRequestHeader::QHttpRequestHeader(const QString &method, const QString &path, int majorVer, int minorVer)
    : QHttpHeader(*new QHttpRequestHeaderPrivate)
{
    Q_D(QHttpRequestHeader);
    d->m = method;
    d->p = path;
    d->majVer = majorVer;
    d->minVer = minorVer;
}

/*!
    Constructs a copy of \a header.
*/
QHttpRequestHeader::QHttpRequestHeader(const QHttpRequestHeader &header)
    : QHttpHeader(*new QHttpRequestHeaderPrivate, header)
{
    Q_D(QHttpRequestHeader);
    d->m = header.d_func()->m;
    d->p = header.d_func()->p;
    d->majVer = header.d_func()->majVer;
    d->minVer = header.d_func()->minVer;
}

/*!
    Copies the content of \a header into this QHttpRequestHeader
*/
QHttpRequestHeader &QHttpRequestHeader::operator=(const QHttpRequestHeader &header)
{
    Q_D(QHttpRequestHeader);
    QHttpHeader::operator=(header);
    d->m = header.d_func()->m;
    d->p = header.d_func()->p;
    d->majVer = header.d_func()->majVer;
    d->minVer = header.d_func()->minVer;
    return *this;
}

/*!
    Constructs a HTTP request header from the string \a str. The \a
    str should consist of one or more "\r\n" delimited lines; the first line
    should be the request-line (format: method, space, request-URI, space
    HTTP-version); each of the remaining lines should have the format key,
    colon, space, value.
*/
QHttpRequestHeader::QHttpRequestHeader(const QString &str)
    : QHttpHeader(*new QHttpRequestHeaderPrivate)
{
    parse(str);
}

/*!
    This function sets the request method to \a method, the
    request-URI to \a path and the protocol-version to \a majorVer and
    \a minorVer. The \a path argument must be properly encoded for an
    HTTP request.

    \sa method() path() majorVersion() minorVersion()
*/
void QHttpRequestHeader::setRequest(const QString &method, const QString &path, int majorVer, int minorVer)
{
    Q_D(QHttpRequestHeader);
    setValid(true);
    d->m = method;
    d->p = path;
    d->majVer = majorVer;
    d->minVer = minorVer;
}

/*!
    Returns the method of the HTTP request header.

    \sa path() majorVersion() minorVersion() setRequest()
*/
QString QHttpRequestHeader::method() const
{
    Q_D(const QHttpRequestHeader);
    return d->m;
}

/*!
    Returns the request-URI of the HTTP request header.

    \sa method() majorVersion() minorVersion() setRequest()
*/
QString QHttpRequestHeader::path() const
{
    Q_D(const QHttpRequestHeader);
    return d->p;
}

/*!
    Returns the major protocol-version of the HTTP request header.

    \sa minorVersion() method() path() setRequest()
*/
int QHttpRequestHeader::majorVersion() const
{
    Q_D(const QHttpRequestHeader);
    return d->majVer;
}

/*!
    Returns the minor protocol-version of the HTTP request header.

    \sa majorVersion() method() path() setRequest()
*/
int QHttpRequestHeader::minorVersion() const
{
    Q_D(const QHttpRequestHeader);
    return d->minVer;
}

/*! \internal
*/
bool QHttpRequestHeader::parseLine(const QString &line, int number)
{
    Q_D(QHttpRequestHeader);
    if (number != 0)
        return QHttpHeader::parseLine(line, number);

    QStringList lst = line.simplified().split(QLatin1String(" "));
    if (lst.count() > 0) {
        d->m = lst[0];
        if (lst.count() > 1) {
            d->p = lst[1];
            if (lst.count() > 2) {
                QString v = lst[2];
                if (v.length() >= 8 && v.left(5) == QLatin1String("HTTP/") &&
                    v[5].isDigit() && v[6] == QLatin1Char('.') && v[7].isDigit()) {
                    d->majVer = v[5].toLatin1() - '0';
                    d->minVer = v[7].toLatin1() - '0';
                    return true;
                }
            }
        }
    }

    return false;
}

/*! \reimp
*/
QString QHttpRequestHeader::toString() const
{
    Q_D(const QHttpRequestHeader);
    QString first(QLatin1String("%1 %2"));
    QString last(QLatin1String(" HTTP/%3.%4\r\n%5\r\n"));
    return first.arg(d->m).arg(d->p) +
        last.arg(d->majVer).arg(d->minVer).arg(QHttpHeader::toString());
}

