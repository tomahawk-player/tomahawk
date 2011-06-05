#ifndef ENTITYTIME_H
#define ENTITYTIME_H

#include <jreen/stanzaextension.h>

#define TOMAHAWK_SIP_MESSAGE_NS QLatin1String("http://www.tomhawk-player.org/sip/transports")

#include "../sipdllmacro.h"

class TomahawkSipMessagePrivate;
class SIPDLLEXPORT TomahawkSipMessage : public Jreen::Payload
{
    J_PAYLOAD(TomahawkSipMessage)
    Q_DECLARE_PRIVATE(TomahawkSipMessage)
    public:
        // sets visible to true
        TomahawkSipMessage(const QString &ip, unsigned int port, const QString &uniqname, const QString &key);

        // sets visible to false as we dont have any extra information
        TomahawkSipMessage();
        ~TomahawkSipMessage();

        const QString ip() const;
        unsigned int port() const;
        const QString uniqname() const;
        const QString key() const;
        bool visible() const;
    private:
        QScopedPointer<TomahawkSipMessagePrivate> d_ptr;
};

#endif // ENTITYTIME_H
