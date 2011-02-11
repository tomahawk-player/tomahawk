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
#include <QDoubleValidator>
#include "oauthtwitter.h"
#include "qtweetplace.h"
#include "qtweetgeosearch.h"
#include "geosearch.h"
#include "qtweetgeocoord.h"
#include "ui_geosearch.h"

GeoSearch::GeoSearch(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GeoSearch)
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

GeoSearch::~GeoSearch()
{
    delete ui;
}

void GeoSearch::changeEvent(QEvent *e)
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

void GeoSearch::onSearchPushButtonClicked()
{
    QTweetGeoSearch *geoSearch = new QTweetGeoSearch(m_oauthTwitter, this);
    QTweetGeoCoord latLong;
    latLong.setLatitude(ui->latitudeLineEdit->text().toDouble());
    latLong.setLongitude(ui->longitudeLineEdit->text().toDouble());

    geoSearch->search(latLong);
    connect(geoSearch, SIGNAL(parsedPlaces(QList<QTweetPlace>)), SLOT(searchPlacesFinished(QList<QTweetPlace>)));
}

void GeoSearch::searchPlacesFinished(const QList<QTweetPlace> &places)
{
    QTweetGeoSearch *geoSearch = qobject_cast<QTweetGeoSearch*>(sender());

    if (geoSearch) {
        ui->tableWidget->clear();
        ui->tableWidget->setRowCount(places.count());
        ui->tableWidget->setColumnCount(2);

        int row = 0;
        foreach (const QTweetPlace& place, places) {
            QTableWidgetItem *fullname = new QTableWidgetItem(place.fullName());
            ui->tableWidget->setItem(row, 0, fullname);
            QTableWidgetItem *placeid = new QTableWidgetItem(place.id());
            ui->tableWidget->setItem(row, 1, placeid);

            ++row;
        }

        geoSearch->deleteLater();

    }
}
