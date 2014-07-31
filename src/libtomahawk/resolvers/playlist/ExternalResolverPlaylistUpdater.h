#ifndef TOMAHAWK_EXTERNALRESOLVERPLAYLISTUPDATER_H
#define TOMAHAWK_EXTERNALRESOLVERPLAYLISTUPDATER_H

#include "playlist/PlaylistUpdaterInterface.h"

namespace Tomahawk {

class ExternalResolverPlaylistUpdater : public PlaylistUpdaterInterface
{
    Q_OBJECT
public:
    explicit ExternalResolverPlaylistUpdater( const playlist_ptr& pl, Resolver* resolver );

    virtual ~ExternalResolverPlaylistUpdater();

    // What type you are. If you add a new updater, add the creation code as well.
    // virtual QString type() const = 0;

    // Small widget to show in playlist header that configures the updater
    // virtual QWidget* configurationWidget() const = 0;

    // Small overlay over playlist icon in the sidebar to indicate that it has this updater type
    // Should be around 16x16 or something
    // virtual QPixmap typeIcon() const { return QPixmap(); }

    // void remove();

    // playlist_ptr playlist() const { return m_playlist; }

    // virtual bool sync() const { return false; }
    // virtual void setSync( bool ) {}

    // virtual bool canSubscribe() const { return false; }
    // virtual bool subscribed() const { return false; }
    // virtual void setSubscribed( bool ) {}
    // virtual void setCollaborative( bool ) {}
    // virtual bool collaborative() const { return false; }

    // The int data value associated with each question must be unique across *all* playlist updaters,
    // as setQuestionResults is called with all questions from all updaters.
    // virtual bool hasCustomDeleter() const { return false; }
    // virtual PlaylistDeleteQuestions deleteQuestions() const { return PlaylistDeleteQuestions(); }
    // virtual void setQuestionResults( const QMap< int, bool > ) {}

public slots:
    // virtual void updateNow() {}

    // void save();

protected:
    // virtual void aboutToDelete() {}

    // QVariantHash settings() const;
    // void saveSettings( const QVariantHash& settings );

signals:

public slots:

};

} // namespace Tomahawk

#endif // TOMAHAWK_EXTERNALRESOLVERPLAYLISTUPDATER_H
