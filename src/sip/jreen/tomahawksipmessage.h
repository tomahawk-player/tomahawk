#ifndef ENTITYTIME_H
#define ENTITYTIME_H

#include <jreen/stanzaextension.h>

#define TOMAHAWK_SIP_MESSAGE_NS QLatin1String("http://www.tomhawk-player.org/sip/transports")

class TomahawkSipMessagePrivate;
class TomahawkSipMessage : public jreen::StanzaExtension
{
    J_EXTENSION(TomahawkSipMessage, "")
    Q_DECLARE_PRIVATE(TomahawkSipMessage)
    public:
        TomahawkSipMessage(QString ip, unsigned int port, QString uniqname, QString key, bool visible);
        // sets visible to false as we dont have any extra information
        TomahawkSipMessage();
        ~TomahawkSipMessage();

        const QString ip() const;
        unsigned int port() const;
        QString uniqname() const;
        QString key() const;
        bool visible() const;
    private:
        QScopedPointer<TomahawkSipMessagePrivate> d_ptr;
};

#endif // ENTITYTIME_H
