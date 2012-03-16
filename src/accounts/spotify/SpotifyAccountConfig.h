#ifndef SPOTIFYACCOUNTCONFIG_H
#define SPOTIFYACCOUNTCONFIG_H

#include <QWidget>

namespace Ui
{
    class SpotifyConfig;
}

namespace Tomahawk
{
namespace Accounts
{

class SpotifyAccount;
struct SpotifyPlaylist;

class SpotifyAccountConfig : public QWidget
{
    Q_OBJECT
public:
    explicit SpotifyAccountConfig( SpotifyAccount* account );

    QString username() const;
    QString password() const;
    bool highQuality() const;

    void setPlaylists( const QList< SpotifyPlaylist* >& playlists );

    void loadFromConfig();
    void saveSettings();

private:
    Ui::SpotifyConfig* m_ui;
    SpotifyAccount* m_account;
    
};

}
}

#endif // SPOTIFYACCOUNTCONFIG_H
