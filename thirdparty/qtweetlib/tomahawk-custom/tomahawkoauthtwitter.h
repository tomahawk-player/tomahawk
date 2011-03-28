#ifndef TOMAHAWKOAUTHTWITTER
#define TOMAHAWKOAUTHTWITTER

#include <oauthtwitter.h>

#include "qtweetlib_global.h"

class QTWEETLIBSHARED_EXPORT TomahawkOAuthTwitter : public OAuthTwitter
{
    Q_OBJECT
    
public:
    TomahawkOAuthTwitter( QObject *parent = 0 );

    ~TomahawkOAuthTwitter() {}
    
protected:
    virtual int authorizationWidget();
    
private slots:
    void error();
};

#endif
