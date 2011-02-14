/* Copyright (c) 2010, Antonie Jovanoski
 *
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact e-mail: Antonie Jovanoski <minimoog77_at_gmail.com>
 */

#include <QNetworkAccessManager>
#include "oauthtwitter.h"
#include "qtweetplace.h"
#include "qtweetgeocoord.h"
#include "qtweetgeoreversegeocode.h"
#include "georeverse.h"
#include "ui_georeverse.h"

GeoReverse::GeoReverse(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GeoReverse)
{
    ui->setupUi(this);

    m_oauthTwitter = new OAuthTwitter(this);
    m_oauthTwitter->setNetworkAccessManager(new QNetworkAccessManager(this));
    m_oauthTwitter->setOAuthToken("");
    m_oauthTwitter->setOAuthTokenSecret("");

    QDoubleValidator *latValidator = new QDoubleValidator(ui->latitudeLineEdit);
    latValidator->setNotation(QDoubleValidator::StandardNotation);
    ui->latitudeLineEdit->setValidator(latValidator);

    QDoubleValidator *longValidator = new QDoubleValidator(ui->longitudeLineEdit);
    longValidator->setNotation(QDoubleValidator::StandardNotation);
    ui->longitudeLineEdit->setValidator(longValidator);

    connect(ui->searchPushButton, SIGNAL(clicked()), SLOT(onSearchPushButtonClicked()));
}

GeoReverse::~GeoReverse()
{
    delete ui;
}

void GeoReverse::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}


void GeoReverse::onSearchPushButtonClicked()
{
    QTweetGeoReverseGeoCode *reverseGeo = new QTweetGeoReverseGeoCode(m_oauthTwitter, this);
    reverseGeo->getPlaces(QTweetGeoCoord(ui->latitudeLineEdit->text().toDouble(),
                          ui->longitudeLineEdit->text().toDouble()));
    connect(reverseGeo, SIGNAL(parsedPlaces(QList<QTweetPlace>)), SLOT(reverseGeoFinished(QList<QTweetPlace>)));
}

void GeoReverse::reverseGeoFinished(const QList<QTweetPlace> &places)
{
    QTweetGeoReverseGeoCode *reverseGeo = qobject_cast<QTweetGeoReverseGeoCode*>(sender());

    if (reverseGeo) {
        ui->tableWidget->clear();
        ui->tableWidget->setRowCount(places.count());
        ui->tableWidget->setColumnCount(4);

        int row = 0;
        foreach (const QTweetPlace& place, places) {
            QTableWidgetItem *fullname = new QTableWidgetItem(place.fullName());
            ui->tableWidget->setItem(row, 0, fullname);
            QTableWidgetItem *placeid = new QTableWidgetItem(place.id());
            ui->tableWidget->setItem(row, 1, placeid);

            //QGeoBoundingBox bb = place.boundingBox();
            //QGeoCoordinate coord = bb.center();

            //approximate, just shows center of the box
            //QTableWidgetItem *lat = new QTableWidgetItem(QString::number(coord.latitude()));
            //ui->tableWidget->setItem(row, 2, lat);
            //QTableWidgetItem *longit = new QTableWidgetItem(QString::number(coord.longitude()));
            //ui->tableWidget->setItem(row, 3, longit);
            //++row;
        }

        reverseGeo->deleteLater();
    }
}
