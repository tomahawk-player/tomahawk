/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Parts copied from qlocale_data_p.h, copyright 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "CountryUtils.h"

QString Tomahawk::CountryUtils::fullCountryFromCode(const QString& countryCode)
{
    ushort uc1 = countryCode[0].toUpper().unicode();
    ushort uc2 = countryCode[1].toUpper().unicode();
    ushort uc3 = QChar('\0').unicode(); // No, not dealign with LatinAmericaAndTheCaribbean for convenience

    const unsigned char *c = country_code_list;
    for (; *c != 0; c += 3) {
        if (uc1 == c[0] && uc2 == c[1] && uc3 == c[2])
        {
            uint country = (uint)((c - country_code_list)/3);

            if (country > uint(QLocale::LastCountry))
                return QLatin1String("Unknown");

            return QString(country_name_list + country_name_index[country]);
        }
    }

    return QString("Unknown");
}