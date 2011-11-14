#ifndef TOMAHAWKOAUTHTWITTERACCOUNT
#define TOMAHAWKOAUTHTWITTERACCOUNT

#include "accounts/accountdllmacro.h"
#include <utils/tomahawkutils.h>

#include <QTweetLib/qtweetlib_global.h>
#include <QTweetLib/oauthtwitter.h>

class ACCOUNTDLLEXPORT TomahawkOAuthTwitter : public OAuthTwitter
{
    Q_OBJECT

public:
    TomahawkOAuthTwitter( QNetworkAccessManager *nam = TomahawkUtils::nam() , QObject *parent = 0 );

    ~TomahawkOAuthTwitter() {}

protected:
    virtual int authorizationWidget();

private slots:
    void error();
};

#endif
