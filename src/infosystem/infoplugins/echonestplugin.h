#ifndef ECHONESTPLUGIN_H
#define ECHONESTPLUGIN_H
#include "tomahawk/infosystem.h"

class QNetworkReply;
namespace Echonest {
class Artist;
}

namespace Tomahawk
{

namespace InfoSystem
{

class EchoNestPlugin : public InfoPlugin
{
    Q_OBJECT
    
public:
    EchoNestPlugin(QObject *parent);
    virtual ~EchoNestPlugin();
    
    void getInfo( const QString &caller, const InfoType type, const QVariant &data, InfoCustomDataHash customData );
    
private:
    void getSongProfile( const QString &caller, const QVariant &data, InfoCustomDataHash &customData, const QString &item = QString() );
    void getArtistBiography ( const QString &caller, const QVariant &data, InfoCustomDataHash &customData );
    void getArtistFamiliarity( const QString &caller, const QVariant &data, InfoCustomDataHash &customData );
    void getArtistHotttnesss( const QString &caller, const QVariant &data, InfoCustomDataHash &customData );
    void getArtistTerms( const QString &caller, const QVariant &data, InfoCustomDataHash &customData );
    void getMiscTopTerms( const QString &caller, const QVariant &data, InfoCustomDataHash &customData );

    bool isValidArtistData( const QString &caller, const QVariant& data, InfoCustomDataHash& customData );
    bool isValidTrackData( const QString &caller, const QVariant& data, InfoCustomDataHash& customData );
    Echonest::Artist artistFromReply( QNetworkReply* );
    
private slots:
    void getArtistBiographySlot();
    void getArtistFamiliaritySlot();
    void getArtistHotttnesssSlot();
    void getArtistTermsSlot();
    void getMiscTopSlot();
    
private:
    QHash< QNetworkReply*, InfoCustomDataHash > m_replyMap;
    QHash< QNetworkReply*, QString > m_callerMap;
};

}

}

#endif // ECHONESTPLUGIN_H
