#include "SpotifyAccountConfig.h"

#include "SpotifyAccount.h"
#include "ui_SpotifyAccountConfig.h"

#include <QListWidget>
#include <QListWidgetItem>

using namespace Tomahawk;
using namespace Accounts;

SpotifyAccountConfig::SpotifyAccountConfig( SpotifyAccount *account )
    : QWidget( 0 )
    , m_ui( new Ui::SpotifyConfig )
    , m_account( account )
{
    m_ui->setupUi( this );

    loadFromConfig();
}


void
SpotifyAccountConfig::loadFromConfig()
{
    m_ui->usernameEdit->setText( m_account->credentials().value( "username" ).toString() );
    m_ui->passwordEdit->setText( m_account->credentials().value( "password" ).toString() );
    m_ui->streamingCheckbox->setChecked( m_account->credentials().value( "highQuality" ).toBool() );
}

void
SpotifyAccountConfig::saveSettings()
{
    for( int i = 0; i < m_ui->playlistList->count(); i++ )
    {
        const QListWidgetItem* item = m_ui->playlistList->itemAt( i, 0 );
        SpotifyPlaylist* pl = item->data( Qt::UserRole ).value< SpotifyPlaylist* >();
        pl->sync = ( item->checkState() == Qt::Checked );
    }
}


QString
SpotifyAccountConfig::username() const
{
    return m_ui->usernameEdit->text().trimmed();
}

QString
SpotifyAccountConfig::password() const
{
    return m_ui->passwordEdit->text().trimmed();
}

bool
SpotifyAccountConfig::highQuality() const
{
    return m_ui->streamingCheckbox->isChecked();
}


void
SpotifyAccountConfig::setPlaylists( const QList<SpotifyPlaylist *>& playlists )
{
    m_ui->playlistList->clear();
    foreach ( SpotifyPlaylist* pl, playlists )
    {
        QListWidgetItem* item = new QListWidgetItem( pl->name, m_ui->playlistList );
        item->setData( Qt::UserRole, QVariant::fromValue< SpotifyPlaylist* >( pl ) );
        item->setFlags( Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled );
        item->setCheckState( pl->sync ? Qt::Checked : Qt::Unchecked );
    }
}
