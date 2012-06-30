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
    MetadataEditor( Tomahawk::result_ptr result, QWidget* parent = 0 );
    ~MetadataEditor() {};

    QString title() { return ui->titleLineEdit->text(); }
    QString artist() { return ui->artistLineEdit->text(); }
    QString album() { return ui->albumLineEdit->text(); }
    int discnumber() { return ui->discNumberSpinBox->value(); }

private slots:
    void writeMetadata();

    void setTitle( const QString& title );
    void setArtist( const QString& artist );
    void setAlbum( const QString& album );
    void setDiscNumber( unsigned int num );
    void setBitrate( unsigned int num );

private:
    Ui::MetadataEditor* ui;

    Tomahawk::result_ptr m_result;
};

#endif // METADATAEDITOR_H
