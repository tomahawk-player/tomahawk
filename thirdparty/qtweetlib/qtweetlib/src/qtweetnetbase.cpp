/* Copyright (c) 2010, Antonie Jovanoski
 *
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact e-mail: Antonie Jovanoski <minimoog77_at_gmail.com>
 */

#include <QtDebug>
#include <QThreadPool>
#include <QNetworkReply>
#include "qtweetnetbase.h"
#include "qtweetstatus.h"
#include "qtweetdmstatus.h"
#include "qtweetuser.h"
#include "qtweetlist.h"
#include "qtweetsearchresult.h"
#include "qtweetsearchpageresults.h"
#include "qtweetplace.h"
#include "qjson/parserrunnable.h"
#include "qjson/parser.h"

/**
 *   Constructor
 */
QTweetNetBase::QTweetNetBase(QObject *parent) :
    QObject(parent), m_oauthTwitter(0), m_jsonParsingEnabled(true), m_authentication(true)
{
}

/**
 *   Constructor
 *   @param oauthTwitter OAuth Twitter
 *   @param parent QObject parent
 */
QTweetNetBase::QTweetNetBase(OAuthTwitter *oauthTwitter, QObject *parent) :
        QObject(parent), m_oauthTwitter(oauthTwitter), m_jsonParsingEnabled(true), m_authentication(true)
{

}

/**
 *  Desctructor
 */
QTweetNetBase::~QTweetNetBase()
{
}

/**
 *   Sets OAuth Twitter authorization object
 *   @param oauthTwitter OAuth Twitter object
 */
void QTweetNetBase::setOAuthTwitter(OAuthTwitter *oauthTwitter)
{
    m_oauthTwitter = oauthTwitter;
}

/**
 *   Gets OAuth Twitter authorization object
 *   @return OAuth Twitter object
 */
OAuthTwitter* QTweetNetBase::oauthTwitter() const
{
    return m_oauthTwitter;
}

/**
 *  Gets response
 */
QByteArray QTweetNetBase::response() const
{
    return m_response;
}

/**
 *  Gets last error message
 */
QString QTweetNetBase::lastErrorMessage() const
{
    return m_lastErrorMessage;
}

/**
 *  Enables/disables json parsing
 *  @remarks When disabled only finished and error signals are emited
 */
void QTweetNetBase::setJsonParsingEnabled(bool enable)
{
    m_jsonParsingEnabled = enable;
}

/**
 *  Checks if its json parsing enabled
 */
bool QTweetNetBase::isJsonParsingEnabled() const
{
    return m_jsonParsingEnabled;
}

/**
 *  Enables/disables oauth authentication
 *  @remarks Most of classes requires authentication
 */
void QTweetNetBase::setAuthenticationEnabled(bool enable)
{
    m_authentication = enable;
}

/**
 *  Checks if authentication is enabled
 */
bool QTweetNetBase::isAuthenticationEnabled() const
{
    return m_authentication;
}

/**
 *  Parses json response
 */
void QTweetNetBase::parseJson(const QByteArray &jsonData)
{
    QJson::ParserRunnable *jsonParser = new QJson::ParserRunnable;
    jsonParser->setData(jsonData);

    connect(jsonParser, SIGNAL(parsingFinished(QVariant,bool,QString)),
            this, SLOT(parsingJsonFinished(QVariant,bool,QString)));

    QThreadPool::globalInstance()->start(jsonParser);
}

/**
 *  Called after response from twitter
 */
void QTweetNetBase::reply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

    if (reply) {
        if (reply->error() == QNetworkReply::NoError) {
            m_response = reply->readAll();
            emit finished(m_response);

            if (isJsonParsingEnabled())
                parseJson(m_response);
        } else {
            m_response = reply->readAll();

            //dump error
            qDebug() << "Network error: " << reply->error();
            qDebug() << "Error string: " << reply->errorString();
            qDebug() << "Error response: " << m_response;

            //HTTP status code
            int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            //try to json parse the error response
            QJson::Parser parser;
            bool ok;

            QVariantMap errMsgMap = parser.parse(m_response, &ok).toMap();
            if (!ok) {
                m_lastErrorMessage.clear();
            } else {
                //QString request = errMsgMap["request"].toString();
                m_lastErrorMessage = errMsgMap["error"].toString();
            }

            switch (httpStatus) {
            case NotModified:
            case BadRequest:
            case Unauthorized:
            case Forbidden:
            case NotFound:
            case NotAcceptable:
            case EnhanceYourCalm:
            case InternalServerError:
            case BadGateway:
            case ServiceUnavailable:
                emit error(static_cast<ErrorCode>(httpStatus), m_lastErrorMessage);
                break;
            default:
                emit error(UnknownError, m_lastErrorMessage);
            }
        }
        reply->deleteLater();
    }
}

/**
 *  Sets last error message
 */
void QTweetNetBase::setLastErrorMessage(const QString &errMsg)
{
    m_lastErrorMessage = errMsg;
}
