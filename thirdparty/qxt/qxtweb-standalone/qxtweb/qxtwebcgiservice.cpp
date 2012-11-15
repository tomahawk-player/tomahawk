/****************************************************************************
 **
 ** Copyright (C) Qxt Foundation. Some rights reserved.
 **
 ** This file is part of the QxtWeb module of the Qxt library.
 **
 ** This library is free software; you can redistribute it and/or modify it
 ** under the terms of the Common Public License, version 1.0, as published
 ** by IBM, and/or under the terms of the GNU Lesser General Public License,
 ** version 2.1, as published by the Free Software Foundation.
 **
 ** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
 ** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
 ** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
 ** FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** You should have received a copy of the CPL and the LGPL along with this
 ** file. See the LICENSE file and the cpl1.0.txt/lgpl-2.1.txt files
 ** included with the source distribution for more information.
 ** If you did not receive a copy of the licenses, contact the Qxt Foundation.
 **
 ** <http://libqxt.org>  <foundation@libqxt.org>
 **
 ****************************************************************************/

/*!
\class QxtWebCgiService

\inmodule QxtWeb

\brief The QxtWebCgiService class provides a CGI/1.1 gateway for QxtWeb

TODO: write docs
TODO: implement timeout
*/

#include "qxtwebcgiservice.h"
#include "qxtwebcgiservice_p.h"
#include "qxtwebevent.h"
#include "qxtwebcontent.h"
#include <QMap>
#include <QFile>
#include <QProcess>
#include <QtDebug>

QxtCgiRequestInfo::QxtCgiRequestInfo() : sessionID(0), requestID(0), eventSent(false), terminateSent(false) {}
QxtCgiRequestInfo::QxtCgiRequestInfo(QxtWebRequestEvent* req) : sessionID(req->sessionID), requestID(req->requestID), eventSent(false), terminateSent(false) {}

/*!
 * Constructs a QxtWebCgiService object with the specified session \a manager and \a parent.
 * This service will invoke the specified \a binary to handle incoming requests.
 *
 * Often, the session manager will also be the parent, but this is not a requirement.
 */
QxtWebCgiService::QxtWebCgiService(const QString& binary, QxtAbstractWebSessionManager* manager, QObject* parent) : QxtAbstractWebService(manager, parent)
{
    QXT_INIT_PRIVATE(QxtWebCgiService);
    qxt_d().binary = binary;
    QObject::connect(&qxt_d().timeoutMapper, SIGNAL(mapped(QObject*)), &qxt_d(), SLOT(terminateProcess(QObject*)));
}

/*!
 * Returns the path to the CGI script that will be executed to handle requests.
 *
 * \sa setBinary()
 */
QString QxtWebCgiService::binary() const
{
    return qxt_d().binary;
}

/*!
 * Sets the path to the CGI script \a bin that will be executed to handle requests.
 *
 * \sa binary()
 */
void QxtWebCgiService::setBinary(const QString& bin)
{
    if (!QFile::exists(bin) || !(QFile::permissions(bin) & (QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther)))
    {
        qWarning() << "QxtWebCgiService::setBinary: " + bin + " does not appear to be executable.";
    }
    qxt_d().binary = bin;
}

/*!
 * Returns the maximum time a CGI script may execute, in milliseconds.
 *
 * The default value is 0, which indicates that CGI scripts will not be terminated
 * due to long running times.
 *
 * \sa setTimeout()
 */
int QxtWebCgiService::timeout() const
{
    return qxt_d().timeout;
}

/*!
 * Sets the maximum \a time a CGI script may execute, in milliseconds.
 *
 * The timer is started when the script is launched. After the timeout elapses once,
 * the script will be asked to stop, as QProcess::terminate(). (That is, the script
 * will receive WM_CLOSE on Windows or SIGTERM on UNIX.) If the process has still
 * failed to terminate after another timeout, it will be forcibly terminated, as
 * QProcess::kill(). (That is, the script will receive TerminateProcess on Windows
 * or SIGKILL on UNIX.)
 *
 * Set the timeout to 0 to disable this behavior; scripts will not be terminated
 * due to excessive run time. This is the default behavior.
 *
 * CAUTION: Keep in mind that the timeout applies to the real running time of the
 * script, not processor time used. A script that initiates a lengthy download 
 * may be interrupted while transferring data to the web browser. To avoid this
 * behavior, see the timeoutOverride property to allow the script to request
 * an extended timeout, or use a different QxtAbstractWebService object for
 * serving streaming content or large files.
 *
 *
 * \sa timeout(), timeoutOverride(), setTimeoutOverride(), QProcess::terminate(), QProcess::kill()
 */
void QxtWebCgiService::setTimeout(int time)
{
    qxt_d().timeout = time;
}

/*!
 * Returns whether or not to allow scripts to override the timeout.
 *
 * \sa setTimeoutOverride(), setTimeout()
 */
bool QxtWebCgiService::timeoutOverride() const
{
    return qxt_d().timeoutOverride;
}

/*!
 * Sets whether or not to allow scripts to override the timeout.
 * Scripts are allowed to override if \a enable is \c true.
 *
 * As an extension to the CGI/1.1 gateway specification, a CGI script may
 * output a "X-QxtWeb-Timeout" header to change the termination timeout
 * on a per-script basis. Only enable this option if you trust the scripts
 * being executed.
 *
 * \sa timeoutOverride(), setTimeout()
 */
void QxtWebCgiService::setTimeoutOverride(bool enable)
{
    qxt_d().timeoutOverride = enable;
}

/*!
 * \reimp
 */
void QxtWebCgiService::pageRequestedEvent(QxtWebRequestEvent* event)
{
    // Create the process object and initialize connections
    QProcess* process = new QProcess(this);
    qxt_d().requests[process] = QxtCgiRequestInfo(event);
    qxt_d().processes[event->content] = process;
    QxtCgiRequestInfo& requestInfo = qxt_d().requests[process];
    QObject::connect(process, SIGNAL(readyRead()), &qxt_d(), SLOT(processReadyRead()));
    QObject::connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), &qxt_d(), SLOT(processFinished()));
    QObject::connect(process, SIGNAL(error(QProcess::ProcessError)), &qxt_d(), SLOT(processFinished()));
    requestInfo.timeout = new QTimer(process);
    qxt_d().timeoutMapper.setMapping(requestInfo.timeout, process);
    QObject::connect(requestInfo.timeout, SIGNAL(timeout()), &qxt_d().timeoutMapper, SLOT(map()));

    // Initialize the system environment
    QStringList s_env = process->systemEnvironment();
    QMap<QString, QString> env;
    foreach(const QString& entry, s_env)
    {
        int pos = entry.indexOf('=');
        env[entry.left(pos)] = entry.mid(pos + 1);
    }

    // Populate CGI/1.1 environment variables
    env["SERVER_SOFTWARE"] = QString("QxtWeb/" QXT_VERSION_STR);
    env["SERVER_NAME"] = event->url.host();
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    if (event->headers.contains("X-Request-Protocol"))
        env["SERVER_PROTOCOL"] = event->headers.value("X-Request-Protocol");
    else
        env.remove("SERVER_PROTOCOL");
    if (event->url.port() != -1)
        env["SERVER_PORT"] = QString::number(event->url.port());
    else
        env.remove("SERVER_PORT");
    env["REQUEST_METHOD"] = event->method;
    env["PATH_INFO"] = event->url.path();
    env["PATH_TRANSLATED"] = event->url.path(); // CGI/1.1 says we should resolve this, but we have no logical interpretation
    env["SCRIPT_NAME"] = event->originalUrl.path().remove(QRegExp(QRegExp::escape(event->url.path()) + '$'));
    env["SCRIPT_FILENAME"] = qxt_d().binary;    // CGI/1.1 doesn't define this but PHP demands it
    env.remove("REMOTE_HOST");
    env["REMOTE_ADDR"] = event->remoteAddress;
    // TODO: If we ever support HTTP authentication, we should use these
    env.remove("AUTH_TYPE");
    env.remove("REMOTE_USER");
    env.remove("REMOTE_IDENT");
    if (event->contentType.isEmpty())
    {
        env.remove("CONTENT_TYPE");
        env.remove("CONTENT_LENGTH");
    }
    else
    {
        env["CONTENT_TYPE"] = event->contentType;
        env["CONTENT_LENGTH"] = QString::number(event->content->unreadBytes());
    }
    env["QUERY_STRING"] = event->url.encodedQuery();

    // Populate HTTP header environment variables
    QMultiHash<QString, QString>::const_iterator iter = event->headers.constBegin();
    while (iter != event->headers.constEnd())
    {
        QString key = "HTTP_" + iter.key().toUpper().replace('-', '_');
        if (key != "HTTP_CONTENT_TYPE" && key != "HTTP_CONTENT_LENGTH")
            env[key] = iter.value();
        iter++;
    }

    // Populate HTTP_COOKIE parameter
    iter = event->cookies.constBegin();
    QString cookies;
    while (iter != event->cookies.constEnd())
    {
        if (!cookies.isEmpty())
            cookies += "; ";
        cookies += iter.key() + '=' + iter.value();
        iter++;
    }
    if (!cookies.isEmpty())
        env["HTTP_COOKIE"] = cookies;

    // Load environment into process space
    QStringList p_env;
    QMap<QString, QString>::iterator env_iter = env.begin();
    while (env_iter != env.end())
    {
        p_env << env_iter.key() + '=' + env_iter.value();
        env_iter++;
    }
    process->setEnvironment(p_env);

    // Launch process
    if (event->url.hasQuery() && event->url.encodedQuery().contains('='))
    {
        // CGI/1.1 spec says to pass the query on the command line if there's no embedded = sign
        process->start(qxt_d().binary + ' ' + QUrl::fromPercentEncoding(event->url.encodedQuery()), QIODevice::ReadWrite);
    }
    else
    {
        process->start(qxt_d().binary, QIODevice::ReadWrite);
    }

    // Start the timeout
    if(qxt_d().timeout > 0)
    {
        requestInfo.timeout->start(qxt_d().timeout);
    }

    // Transmit POST data
    if (event->content)
    {
        QObject::connect(event->content, SIGNAL(readyRead()), &qxt_d(), SLOT(browserReadyRead()));
        qxt_d().browserReadyRead(event->content);
    }
}

/*!
 * \internal
 */
void QxtWebCgiServicePrivate::browserReadyRead(QObject* o_content)
{
    if (!o_content) o_content = sender();
    QxtWebContent* content = static_cast<QxtWebContent*>(o_content); // this is a private class, no worries about type safety

    // Read POST data and copy it to the process
    QByteArray data = content->readAll();
    if (!data.isEmpty())
        processes[content]->write(data);

    // If no POST data remains unsent, clean up
    if (!content->unreadBytes() && processes.contains(content))
    {
        processes[content]->closeWriteChannel();
        processes.remove(content);
    }
}

/*!
 * \internal
 */
void QxtWebCgiServicePrivate::processReadyRead()
{
    QProcess* process = static_cast<QProcess*>(sender());
    QxtCgiRequestInfo& request = requests[process];

    QByteArray line;
    while (process->canReadLine())
    {
        // Read in a CGI/1.1 header line
        line = process->readLine().replace(QByteArray("\r"), ""); //krazy:exclude=doublequote_chars
        if (line == "\n")
        {
            // An otherwise-empty line indicates the end of CGI/1.1 headers and the start of content
            QObject::disconnect(process, SIGNAL(readyRead()), this, 0);
            QxtWebPageEvent* event = 0;
            int code = 200;
            if (request.headers.contains("status"))
            {
                // CGI/1.1 defines a "Status:" header that dictates the HTTP response code
                code = request.headers["status"].left(3).toInt();
                if (code >= 300 && code < 400)  // redirect
                {
                    event = new QxtWebRedirectEvent(request.sessionID, request.requestID, request.headers["location"], code);
                }
            }
            // If a previous header (currently just status) hasn't created an event, create a normal page event here
            if (!event)
            {
                event = new QxtWebPageEvent(request.sessionID, request.requestID, QSharedPointer<QIODevice>(process) );
                event->status = code;
            }
            // Add other response headers passed from CGI (currently only Content-Type is supported)
            if (request.headers.contains("content-type"))
                event->contentType = request.headers["content-type"].toUtf8();
            // TODO: QxtWeb doesn't support transmitting arbitrary HTTP headers right now, but it may be desirable
            // for applications that know what kind of server frontend they're using to allow scripts to send
            // protocol-specific headers.
            
            // Post the event
            qxt_p().postEvent(event);
            request.eventSent = true;
            return;
        }
        else
        {
            // Since we haven't reached the end of headers yet, parse a header
            int pos = line.indexOf(": ");
            QByteArray hdrName = line.left(pos).toLower();
            QByteArray hdrValue = line.mid(pos + 2).replace(QByteArray("\n"), ""); //krazy:exclude=doublequote_chars
            if (hdrName == "set-cookie")
            {
                // Parse a new cookie and post an event to send it to the client
                QList<QByteArray> cookies = hdrValue.split(',');
                foreach(const QByteArray& cookie, cookies)
                {
                    int equals = cookie.indexOf("=");
                    int semi = cookie.indexOf(";");
                    QByteArray cookieName = cookie.left(equals);
                    int age = cookie.toLower().indexOf("max-age=", semi);
                    int secs = -1;
                    if (age >= 0)
                        secs = cookie.mid(age + 8, cookie.indexOf(";", age) - age - 8).toInt();
                    if (secs == 0)
                    {
                        qxt_p().postEvent(new QxtWebRemoveCookieEvent(request.sessionID, cookieName));
                    }
                    else
                    {
                        QByteArray cookieValue = cookie.mid(equals + 1, semi - equals - 1);
                        QDateTime cookieExpires;
                        if (secs != -1)
                            cookieExpires = QDateTime::currentDateTime().addSecs(secs);
                        qxt_p().postEvent(new QxtWebStoreCookieEvent(request.sessionID, cookieName, cookieValue, cookieExpires));
                    }
                }
            }
            else if(hdrName == "x-qxtweb-timeout")
            {
                if(timeoutOverride)
                    request.timeout->setInterval(hdrValue.toInt());
            }
            else
            {
                // Store other headers for later inspection
                request.headers[hdrName] = hdrValue;
            }
        }
    }
}

/*!
 * \internal
 */
void QxtWebCgiServicePrivate::processFinished()
{
    QProcess* process = static_cast<QProcess*>(sender());
    QxtCgiRequestInfo& request = requests[process];

    if (!request.eventSent)
    {
        // If no event was posted, issue an internal error
        qxt_p().postEvent(new QxtWebErrorEvent(request.sessionID, request.requestID, 500, "Internal Server Error"));
    }

    // Clean up data structures
    process->close();
    QxtWebContent* key = processes.key(process);
    if (key) processes.remove(key);
    timeoutMapper.removeMappings(request.timeout);
    requests.remove(process);
}

/*!
 * \internal
 */
void QxtWebCgiServicePrivate::terminateProcess(QObject* o_process)
{
    QProcess* process = static_cast<QProcess*>(o_process);
    QxtCgiRequestInfo& request = requests[process];

    if(request.terminateSent)
    {
        // kill with fire
        process->kill();
    }
    else
    {
        // kill nicely
        process->terminate();
        request.terminateSent = true;
    }
}
