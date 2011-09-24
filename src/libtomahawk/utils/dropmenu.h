#ifndef DROPMENU_H
#define DROPMENU_H

#include "dllmacro.h"
#include "dropjob.h"

#include <QWidget>
#include <QLabel>

class DropMenuEntry;

class DLLEXPORT DropMenu: public QWidget
{
    Q_OBJECT
public:
    /** @brief Create a DropMenu with the given default flags if an invalid/empty area is hovered */
    explicit DropMenu( QWidget *parent = 0 );

    void addEntry( DropMenuEntry *entry, bool isDefault = false );

    void setFilter( DropJob::DropFlags shownEntries );


    /** @brief Returns true if the mouse is somewhere over the contained entries */
    bool hovered() const;

    DropMenuEntry *activeEntry();

signals:
    void dropReceived( QDropEvent *event );
    void mouseLeft();

private slots:
    void entryHovered( DropMenuEntry* entry );
    void entryLeft( DropMenuEntry* entry );

private:
    QList< DropMenuEntry* > m_entries;
    DropJob::DropFlags m_defaultFlags;
    DropMenuEntry *m_defaultEntry;
    DropMenuEntry *m_activeEntry;
};

class DLLEXPORT DropMenuEntry : public QWidget
{
    Q_OBJECT
public:
    explicit DropMenuEntry( const QPixmap &icon, const QString &text, DropJob::DropFlags flags, QWidget *parent = 0 );

    void setActive( bool active );
    bool hovered() const;
    DropJob::DropFlags dropFlags() const;

signals:
    void dropReceived( QDropEvent *event );
    void mouseEntered( DropMenuEntry *entry );
    void mouseLeft( DropMenuEntry *entry );

public slots:

protected:
    virtual void dragEnterEvent( QDragEnterEvent *event );
    virtual void dragLeaveEvent( QDragLeaveEvent *event );
    virtual void dropEvent( QDropEvent *event );

private:
    bool m_hovered;
    QLabel *m_label;
    DropJob::DropFlags m_flags;

};

#endif // DROPMENU_H
