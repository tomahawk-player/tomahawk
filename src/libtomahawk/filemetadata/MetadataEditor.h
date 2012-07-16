/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Christopher Reichert <creichert07@gmail.com>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef METADATAEDITOR_H
#define METADATAEDITOR_H

#include <QtGui/QDialog>
#include <QtGui/QLineEdit>
#include <QtGui/QSpinBox>

#include "ui_MetadataEditor.h"
#include "Result.h"
#include "Typedefs.h"

class QString;


class MetadataEditor : public QDialog
{
Q_OBJECT

public:
    MetadataEditor( const Tomahawk::result_ptr& result, QWidget* parent = 0 );
    ~MetadataEditor() {};

protected:
    QString title() const { return ui->titleLineEdit->text(); }
    QString artist() const { return ui->artistLineEdit->text(); }
    QString album() const { return ui->albumLineEdit->text(); }
    int discnumber() const { return ui->discNumberSpinBox->value(); }
    int year() const { return ui->yearSpinBox->value(); }
    int bitrate() const { return ui->bitrateSpinBox->value(); }
    void loadResult( const Tomahawk::result_ptr& result );

private slots:
    void writeMetadata( bool closeDlg = false );
    void enablePushButtons();
    void loadNextResult();
    void loadPreviousResult();

    /* tag attributes */
    void setTitle( const QString& title );
    void setArtist( const QString& artist );
    void setAlbum( const QString& album );
    void setDiscNumber( unsigned int num );
    void setDuration( unsigned int duration );
    void setYear( int year );
    void setBitrate( unsigned int num );

    /* file attributes */
    void setFileName( const QString& fn );
    void setFileSize( const QString& size );

private:
    Ui::MetadataEditor* ui;

    Tomahawk::result_ptr m_result;
    Tomahawk::playlistinterface_ptr m_interface;
    QStringList m_editFiles;
};

#endif // METADATAEDITOR_H
