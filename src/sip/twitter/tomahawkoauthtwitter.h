#ifndef TOMAHAWKOAUTHTWITTER
#define TOMAHAWKOAUTHTWITTER

#include <oauthtwitter.h>

class TomahawkOAuthTwitter : public OAuthTwitter
{
    Q_OBJECT
    
public:
    TomahawkOAuthTwitter(QObject *parent = 0)
        : OAuthTwitter( parent )
        {}
        
    ~TomahawkOAuthTwitter() {}
    
protected:
    virtual int authorizationWidget();
};

#endif
