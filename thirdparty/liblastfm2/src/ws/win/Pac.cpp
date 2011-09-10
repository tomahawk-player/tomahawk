/*
   Copyright 2009 Last.fm Ltd. 
      - Primarily authored by Max Howell, Jono Cole and Doug Mansell

   This file is part of liblastfm.

   liblastfm is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   liblastfm is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with liblastfm.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "Pac.h"
#include <QNetworkRequest>
#include <QStringList>
#include <QUrl>
#include <winhttp.h>
//#include <atlbase.h>
//#include <atlconv.h>


static bool
parsePacServer(const QString &s, QNetworkProxy &p)
{
    // remove optional leading "scheme=" portion
    int start = s.indexOf('=');
    QUrl url(s.mid(start+1), QUrl::TolerantMode);

    if (url.isValid())
    {
        p.setHostName(url.host());
        p.setPort(url.port());
        return true;
    }
    return false;
}


static QList<QNetworkProxy>
parsePacResult(const QString &pacResult)
{
    // msdn says: "The proxy server list contains one or more of the 
    // following strings separated by semicolons or whitespace."
    // ([<scheme>=][<scheme>"://"]<server>[":"<port>])

    QList<QNetworkProxy> result;
    QStringList proxies = pacResult.split(QRegExp("[\\s;]"), QString::SkipEmptyParts);
    foreach(const QString &s, proxies)
    {
        QNetworkProxy proxy( QNetworkProxy::HttpProxy );
        if (parsePacServer(s, proxy))
        {
            result << proxy;
        }
    }
    return result;
}


////////////////


lastfm::Pac::Pac()
    : m_bFailed( false )
    , m_hSession( 0 )
{}

lastfm::Pac::~Pac()
{
/*    if (m_hSession)
        WinHttpCloseHandle(m_hSession);*/
}

QNetworkProxy
lastfm::Pac::resolve(const QNetworkRequest &request, const wchar_t* pacUrl)
{
    QNetworkProxy out;
    if (m_bFailed) return out;

    if (!m_hSession)
    {
        QByteArray user_agent = request.rawHeader("user-agent");
        //m_hSession = WinHttpOpen(CA2W(user_agent), WINHTTP_ACCESS_TYPE_NO_PROXY, 0, 0, WINHTTP_FLAG_ASYNC);
    }
    if (m_hSession)
    {
        WINHTTP_PROXY_INFO info;
        WINHTTP_AUTOPROXY_OPTIONS opts;
        memset(&opts, 0, sizeof(opts));
        if (pacUrl) 
        {
            // opts.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
            // opts.lpszAutoConfigUrl = pacUrl;
        } 
        else
        {
            // opts.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
            // opts.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
        }
        opts.fAutoLogonIfChallenged = TRUE;
        
        if (WinHttpGetProxyForUrl(m_hSession, (const WCHAR*)request.url().toString().utf16(), &opts, &info)) {
            if (info.lpszProxy) 
            {
                QList<QNetworkProxy> proxies = parsePacResult(QString::fromUtf16((const ushort*)info.lpszProxy));
                if (!proxies.empty())
                {
                    out = proxies.at(0);
                }
                GlobalFree((void*)info.lpszProxy);
            }
            if (info.lpszProxyBypass)
            {
                GlobalFree((void*)info.lpszProxyBypass);
            }
        } else {
            m_bFailed = true;
        }
    }

    return out;
}
