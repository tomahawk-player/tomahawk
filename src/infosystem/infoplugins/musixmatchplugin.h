#ifndef MUSIXMATCHPLUGIN_H
#define MUSIXMATCHPLUGIN_H
#include "tomahawk/infosystem.h"

class QNetworkReply;

namespace Tomahawk
{

namespace InfoSystem
{

class MusixMatchPlugin : public InfoPlugin
{
    Q_OBJECT
    
public:
    MusixMatchPlugin(QObject *parent);
    virtual ~MusixMatchPlugin();
    
    void getInfo(const QString &caller, const InfoType type, const QVariant &data, InfoCustomDataHash customData);
    
private:
    bool isValidTrackData( const QString &caller, const QVariant& data, InfoCustomDataHash &customData );
    
public slots:
    void trackSearchSlot();
    void trackLyricsSlot();

private:
    QString m_apiKey;
};

}

}

#endif // MUSIXMATCHPLUGIN_H
