#ifndef TOMAHAWKOAUTHTWITTER
#define TOMAHAWKOAUTHTWITTER

#include <oauthtwitter.h>

#include "../sipdllmacro.h"

class SIPDLLEXPORT TomahawkOAuthTwitter : public OAuthTwitter
{
    Q_OBJECT
    
public:
    TomahawkOAuthTwitter( QObject *parent = 0 );
        
    ~TomahawkOAuthTwitter() {}
    
protected:
    virtual int authorizationWidget();
};

#endif
