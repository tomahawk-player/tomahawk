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

class SpotifyAccountConfig : public QWidget
{
    Q_OBJECT
public:
    explicit SpotifyAccountConfig( SpotifyAccount* account );

    QString username() const;
    QString password() const;
    bool highQuality() const;

    QStringList playlistsToSync() const;

    void loadFromConfig();

private:
    Ui::SpotifyConfig* m_ui;
    SpotifyAccount* m_account;
    
};

}
}

#endif // SPOTIFYACCOUNTCONFIG_H
