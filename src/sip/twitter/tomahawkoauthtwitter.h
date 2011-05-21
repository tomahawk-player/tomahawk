#ifndef TOMAHAWKOAUTHTWITTER
#define TOMAHAWKOAUTHTWITTER

#include "../sipdllmacro.h"

#include <qtweetlib_global.h>
#include <oauthtwitter.h>

class SIPDLLEXPORT TomahawkOAuthTwitter : public OAuthTwitter
{
    Q_OBJECT

public:
    TomahawkOAuthTwitter( QNetworkAccessManager *nam, QObject *parent = 0 );

    ~TomahawkOAuthTwitter() {}

protected:
    virtual int authorizationWidget();

private slots:
    void error();
};

#endif
