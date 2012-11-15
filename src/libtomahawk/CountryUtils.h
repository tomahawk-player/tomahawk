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
#ifndef CHARTSPLUGIN_DATA_P_H
#define CHARTSPLUGIN_DATA_P_H

#include "DllMacro.h"

#include <QtCore/qchar.h>
#include <QLocale>

namespace Tomahawk {
namespace CountryUtils {

static const unsigned char country_code_list[] =
"  \0" // AnyCountry
"AF\0" // Afghanistan
"AL\0" // Albania
"DZ\0" // Algeria
"AS\0" // AmericanSamoa
"AD\0" // Andorra
"AO\0" // Angola
"AI\0" // Anguilla
"AQ\0" // Antarctica
"AG\0" // AntiguaAndBarbuda
"AR\0" // Argentina
"AM\0" // Armenia
"AW\0" // Aruba
"AU\0" // Australia
"AT\0" // Austria
"AZ\0" // Azerbaijan
"BS\0" // Bahamas
"BH\0" // Bahrain
"BD\0" // Bangladesh
"BB\0" // Barbados
"BY\0" // Belarus
"BE\0" // Belgium
"BZ\0" // Belize
"BJ\0" // Benin
"BM\0" // Bermuda
"BT\0" // Bhutan
"BO\0" // Bolivia
"BA\0" // BosniaAndHerzegowina
"BW\0" // Botswana
"BV\0" // BouvetIsland
"BR\0" // Brazil
"IO\0" // BritishIndianOceanTerritory
"BN\0" // BruneiDarussalam
"BG\0" // Bulgaria
"BF\0" // BurkinaFaso
"BI\0" // Burundi
"KH\0" // Cambodia
"CM\0" // Cameroon
"CA\0" // Canada
"CV\0" // CapeVerde
"KY\0" // CaymanIslands
"CF\0" // CentralAfricanRepublic
"TD\0" // Chad
"CL\0" // Chile
"CN\0" // China
"CX\0" // ChristmasIsland
"CC\0" // CocosIslands
"CO\0" // Colombia
"KM\0" // Comoros
"CD\0" // DemocraticRepublicOfCongo
"CG\0" // PeoplesRepublicOfCongo
"CK\0" // CookIslands
"CR\0" // CostaRica
"CI\0" // IvoryCoast
"HR\0" // Croatia
"CU\0" // Cuba
"CY\0" // Cyprus
"CZ\0" // CzechRepublic
"DK\0" // Denmark
"DJ\0" // Djibouti
"DM\0" // Dominica
"DO\0" // DominicanRepublic
"TL\0" // EastTimor
"EC\0" // Ecuador
"EG\0" // Egypt
"SV\0" // ElSalvador
"GQ\0" // EquatorialGuinea
"ER\0" // Eritrea
"EE\0" // Estonia
"ET\0" // Ethiopia
"FK\0" // FalklandIslands
"FO\0" // FaroeIslands
"FJ\0" // Fiji
"FI\0" // Finland
"FR\0" // France
"FX\0" // MetropolitanFrance
"GF\0" // FrenchGuiana
"PF\0" // FrenchPolynesia
"TF\0" // FrenchSouthernTerritories
"GA\0" // Gabon
"GM\0" // Gambia
"GE\0" // Georgia
"DE\0" // Germany
"GH\0" // Ghana
"GI\0" // Gibraltar
"GR\0" // Greece
"GL\0" // Greenland
"GD\0" // Grenada
"GP\0" // Guadeloupe
"GU\0" // Guam
"GT\0" // Guatemala
"GN\0" // Guinea
"GW\0" // GuineaBissau
"GY\0" // Guyana
"HT\0" // Haiti
"HM\0" // HeardAndMcDonaldIslands
"HN\0" // Honduras
"HK\0" // HongKong
"HU\0" // Hungary
"IS\0" // Iceland
"IN\0" // India
"ID\0" // Indonesia
"IR\0" // Iran
"IQ\0" // Iraq
"IE\0" // Ireland
"IL\0" // Israel
"IT\0" // Italy
"JM\0" // Jamaica
"JP\0" // Japan
"JO\0" // Jordan
"KZ\0" // Kazakhstan
"KE\0" // Kenya
"KI\0" // Kiribati
"KP\0" // DemocraticRepublicOfKorea
"KR\0" // RepublicOfKorea
"KW\0" // Kuwait
"KG\0" // Kyrgyzstan
"LA\0" // Lao
"LV\0" // Latvia
"LB\0" // Lebanon
"LS\0" // Lesotho
"LR\0" // Liberia
"LY\0" // LibyanArabJamahiriya
"LI\0" // Liechtenstein
"LT\0" // Lithuania
"LU\0" // Luxembourg
"MO\0" // Macau
"MK\0" // Macedonia
"MG\0" // Madagascar
"MW\0" // Malawi
"MY\0" // Malaysia
"MV\0" // Maldives
"ML\0" // Mali
"MT\0" // Malta
"MH\0" // MarshallIslands
"MQ\0" // Martinique
"MR\0" // Mauritania
"MU\0" // Mauritius
"YT\0" // Mayotte
"MX\0" // Mexico
"FM\0" // Micronesia
"MD\0" // Moldova
"MC\0" // Monaco
"MN\0" // Mongolia
"MS\0" // Montserrat
"MA\0" // Morocco
"MZ\0" // Mozambique
"MM\0" // Myanmar
"NA\0" // Namibia
"NR\0" // Nauru
"NP\0" // Nepal
"NL\0" // Netherlands
"AN\0" // NetherlandsAntilles
"NC\0" // NewCaledonia
"NZ\0" // NewZealand
"NI\0" // Nicaragua
"NE\0" // Niger
"NG\0" // Nigeria
"NU\0" // Niue
"NF\0" // NorfolkIsland
"MP\0" // NorthernMarianaIslands
"NO\0" // Norway
"OM\0" // Oman
"PK\0" // Pakistan
"PW\0" // Palau
"PS\0" // PalestinianTerritory
"PA\0" // Panama
"PG\0" // PapuaNewGuinea
"PY\0" // Paraguay
"PE\0" // Peru
"PH\0" // Philippines
"PN\0" // Pitcairn
"PL\0" // Poland
"PT\0" // Portugal
"PR\0" // PuertoRico
"QA\0" // Qatar
"RE\0" // Reunion
"RO\0" // Romania
"RU\0" // RussianFederation
"RW\0" // Rwanda
"KN\0" // SaintKittsAndNevis
"LC\0" // StLucia
"VC\0" // StVincentAndTheGrenadines
"WS\0" // Samoa
"SM\0" // SanMarino
"ST\0" // SaoTomeAndPrincipe
"SA\0" // SaudiArabia
"SN\0" // Senegal
"SC\0" // Seychelles
"SL\0" // SierraLeone
"SG\0" // Singapore
"SK\0" // Slovakia
"SI\0" // Slovenia
"SB\0" // SolomonIslands
"SO\0" // Somalia
"ZA\0" // SouthAfrica
"GS\0" // SouthGeorgiaAndTheSouthSandwichIslands
"ES\0" // Spain
"LK\0" // SriLanka
"SH\0" // StHelena
"PM\0" // StPierreAndMiquelon
"SD\0" // Sudan
"SR\0" // Suriname
"SJ\0" // SvalbardAndJanMayenIslands
"SZ\0" // Swaziland
"SE\0" // Sweden
"CH\0" // Switzerland
"SY\0" // SyrianArabRepublic
"TW\0" // Taiwan
"TJ\0" // Tajikistan
"TZ\0" // Tanzania
"TH\0" // Thailand
"TG\0" // Togo
"TK\0" // Tokelau
"TO\0" // Tonga
"TT\0" // TrinidadAndTobago
"TN\0" // Tunisia
"TR\0" // Turkey
"TM\0" // Turkmenistan
"TC\0" // TurksAndCaicosIslands
"TV\0" // Tuvalu
"UG\0" // Uganda
"UA\0" // Ukraine
"AE\0" // UnitedArabEmirates
"GB\0" // UnitedKingdom
"US\0" // UnitedStates
"UM\0" // UnitedStatesMinorOutlyingIslands
"UY\0" // Uruguay
"UZ\0" // Uzbekistan
"VU\0" // Vanuatu
"VA\0" // VaticanCityState
"VE\0" // Venezuela
"VN\0" // VietNam
"VG\0" // BritishVirginIslands
"VI\0" // USVirginIslands
"WF\0" // WallisAndFutunaIslands
"EH\0" // WesternSahara
"YE\0" // Yemen
"YU\0" // Yugoslavia
"ZM\0" // Zambia
"ZW\0" // Zimbabwe
"CS\0" // SerbiaAndMontenegro
"ME\0" // Montenegro
"RS\0" // Serbia
"BL\0" // Saint Barthelemy
"MF\0" // Saint Martin
"419" // LatinAmericaAndTheCaribbean
;


static const char country_name_list[] =
"Default\0"
"Afghanistan\0"
"Albania\0"
"Algeria\0"
"AmericanSamoa\0"
"Andorra\0"
"Angola\0"
"Anguilla\0"
"Antarctica\0"
"AntiguaAndBarbuda\0"
"Argentina\0"
"Armenia\0"
"Aruba\0"
"Australia\0"
"Austria\0"
"Azerbaijan\0"
"Bahamas\0"
"Bahrain\0"
"Bangladesh\0"
"Barbados\0"
"Belarus\0"
"Belgium\0"
"Belize\0"
"Benin\0"
"Bermuda\0"
"Bhutan\0"
"Bolivia\0"
"BosniaAndHerzegowina\0"
"Botswana\0"
"BouvetIsland\0"
"Brazil\0"
"BritishIndianOceanTerritory\0"
"BruneiDarussalam\0"
"Bulgaria\0"
"BurkinaFaso\0"
"Burundi\0"
"Cambodia\0"
"Cameroon\0"
"Canada\0"
"CapeVerde\0"
"CaymanIslands\0"
"CentralAfricanRepublic\0"
"Chad\0"
"Chile\0"
"China\0"
"ChristmasIsland\0"
"CocosIslands\0"
"Colombia\0"
"Comoros\0"
"DemocraticRepublicOfCongo\0"
"PeoplesRepublicOfCongo\0"
"CookIslands\0"
"CostaRica\0"
"IvoryCoast\0"
"Croatia\0"
"Cuba\0"
"Cyprus\0"
"CzechRepublic\0"
"Denmark\0"
"Djibouti\0"
"Dominica\0"
"DominicanRepublic\0"
"EastTimor\0"
"Ecuador\0"
"Egypt\0"
"ElSalvador\0"
"EquatorialGuinea\0"
"Eritrea\0"
"Estonia\0"
"Ethiopia\0"
"FalklandIslands\0"
"FaroeIslands\0"
"Fiji\0"
"Finland\0"
"France\0"
"MetropolitanFrance\0"
"FrenchGuiana\0"
"FrenchPolynesia\0"
"FrenchSouthernTerritories\0"
"Gabon\0"
"Gambia\0"
"Georgia\0"
"Germany\0"
"Ghana\0"
"Gibraltar\0"
"Greece\0"
"Greenland\0"
"Grenada\0"
"Guadeloupe\0"
"Guam\0"
"Guatemala\0"
"Guinea\0"
"GuineaBissau\0"
"Guyana\0"
"Haiti\0"
"HeardAndMcDonaldIslands\0"
"Honduras\0"
"HongKong\0"
"Hungary\0"
"Iceland\0"
"India\0"
"Indonesia\0"
"Iran\0"
"Iraq\0"
"Ireland\0"
"Israel\0"
"Italy\0"
"Jamaica\0"
"Japan\0"
"Jordan\0"
"Kazakhstan\0"
"Kenya\0"
"Kiribati\0"
"DemocraticRepublicOfKorea\0"
"RepublicOfKorea\0"
"Kuwait\0"
"Kyrgyzstan\0"
"Lao\0"
"Latvia\0"
"Lebanon\0"
"Lesotho\0"
"Liberia\0"
"LibyanArabJamahiriya\0"
"Liechtenstein\0"
"Lithuania\0"
"Luxembourg\0"
"Macau\0"
"Macedonia\0"
"Madagascar\0"
"Malawi\0"
"Malaysia\0"
"Maldives\0"
"Mali\0"
"Malta\0"
"MarshallIslands\0"
"Martinique\0"
"Mauritania\0"
"Mauritius\0"
"Mayotte\0"
"Mexico\0"
"Micronesia\0"
"Moldova\0"
"Monaco\0"
"Mongolia\0"
"Montserrat\0"
"Morocco\0"
"Mozambique\0"
"Myanmar\0"
"Namibia\0"
"Nauru\0"
"Nepal\0"
"Netherlands\0"
"NetherlandsAntilles\0"
"NewCaledonia\0"
"NewZealand\0"
"Nicaragua\0"
"Niger\0"
"Nigeria\0"
"Niue\0"
"NorfolkIsland\0"
"NorthernMarianaIslands\0"
"Norway\0"
"Oman\0"
"Pakistan\0"
"Palau\0"
"PalestinianTerritory\0"
"Panama\0"
"PapuaNewGuinea\0"
"Paraguay\0"
"Peru\0"
"Philippines\0"
"Pitcairn\0"
"Poland\0"
"Portugal\0"
"PuertoRico\0"
"Qatar\0"
"Reunion\0"
"Romania\0"
"RussianFederation\0"
"Rwanda\0"
"SaintKittsAndNevis\0"
"StLucia\0"
"StVincentAndTheGrenadines\0"
"Samoa\0"
"SanMarino\0"
"SaoTomeAndPrincipe\0"
"SaudiArabia\0"
"Senegal\0"
"Seychelles\0"
"SierraLeone\0"
"Singapore\0"
"Slovakia\0"
"Slovenia\0"
"SolomonIslands\0"
"Somalia\0"
"SouthAfrica\0"
"SouthGeorgiaAndTheSouthSandwichIslands\0"
"Spain\0"
"SriLanka\0"
"StHelena\0"
"StPierreAndMiquelon\0"
"Sudan\0"
"Suriname\0"
"SvalbardAndJanMayenIslands\0"
"Swaziland\0"
"Sweden\0"
"Switzerland\0"
"SyrianArabRepublic\0"
"Taiwan\0"
"Tajikistan\0"
"Tanzania\0"
"Thailand\0"
"Togo\0"
"Tokelau\0"
"Tonga\0"
"TrinidadAndTobago\0"
"Tunisia\0"
"Turkey\0"
"Turkmenistan\0"
"TurksAndCaicosIslands\0"
"Tuvalu\0"
"Uganda\0"
"Ukraine\0"
"UnitedArabEmirates\0"
"UnitedKingdom\0"
"UnitedStates\0"
"UnitedStatesMinorOutlyingIslands\0"
"Uruguay\0"
"Uzbekistan\0"
"Vanuatu\0"
"VaticanCityState\0"
"Venezuela\0"
"VietNam\0"
"BritishVirginIslands\0"
"USVirginIslands\0"
"WallisAndFutunaIslands\0"
"WesternSahara\0"
"Yemen\0"
"Yugoslavia\0"
"Zambia\0"
"Zimbabwe\0"
"SerbiaAndMontenegro\0"
"Montenegro\0"
"Serbia\0"
"Saint Barthelemy\0"
"Saint Martin\0"
"LatinAmericaAndTheCaribbean\0"
;

static const quint16 country_name_index[] = {
    0, // AnyCountry
    8, // Afghanistan
    20, // Albania
    28, // Algeria
    36, // AmericanSamoa
    50, // Andorra
    58, // Angola
    65, // Anguilla
    74, // Antarctica
    85, // AntiguaAndBarbuda
    103, // Argentina
    113, // Armenia
    121, // Aruba
    127, // Australia
    137, // Austria
    145, // Azerbaijan
    156, // Bahamas
    164, // Bahrain
    172, // Bangladesh
    183, // Barbados
    192, // Belarus
    200, // Belgium
    208, // Belize
    215, // Benin
    221, // Bermuda
    229, // Bhutan
    236, // Bolivia
    244, // BosniaAndHerzegowina
    265, // Botswana
    274, // BouvetIsland
    287, // Brazil
    294, // BritishIndianOceanTerritory
    322, // BruneiDarussalam
    339, // Bulgaria
    348, // BurkinaFaso
    360, // Burundi
    368, // Cambodia
    377, // Cameroon
    386, // Canada
    393, // CapeVerde
    403, // CaymanIslands
    417, // CentralAfricanRepublic
    440, // Chad
    445, // Chile
    451, // China
    457, // ChristmasIsland
    473, // CocosIslands
    486, // Colombia
    495, // Comoros
    503, // DemocraticRepublicOfCongo
    529, // PeoplesRepublicOfCongo
    552, // CookIslands
    564, // CostaRica
    574, // IvoryCoast
    585, // Croatia
    593, // Cuba
    598, // Cyprus
    605, // CzechRepublic
    619, // Denmark
    627, // Djibouti
    636, // Dominica
    645, // DominicanRepublic
    663, // EastTimor
    673, // Ecuador
    681, // Egypt
    687, // ElSalvador
    698, // EquatorialGuinea
    715, // Eritrea
    723, // Estonia
    731, // Ethiopia
    740, // FalklandIslands
    756, // FaroeIslands
    769, // Fiji
    774, // Finland
    782, // France
    789, // MetropolitanFrance
    808, // FrenchGuiana
    821, // FrenchPolynesia
    837, // FrenchSouthernTerritories
    863, // Gabon
    869, // Gambia
    876, // Georgia
    884, // Germany
    892, // Ghana
    898, // Gibraltar
    908, // Greece
    915, // Greenland
    925, // Grenada
    933, // Guadeloupe
    944, // Guam
    949, // Guatemala
    959, // Guinea
    966, // GuineaBissau
    979, // Guyana
    986, // Haiti
    992, // HeardAndMcDonaldIslands
    1016, // Honduras
    1025, // HongKong
    1034, // Hungary
    1042, // Iceland
    1050, // India
    1056, // Indonesia
    1066, // Iran
    1071, // Iraq
    1076, // Ireland
    1084, // Israel
    1091, // Italy
    1097, // Jamaica
    1105, // Japan
    1111, // Jordan
    1118, // Kazakhstan
    1129, // Kenya
    1135, // Kiribati
    1144, // DemocraticRepublicOfKorea
    1170, // RepublicOfKorea
    1186, // Kuwait
    1193, // Kyrgyzstan
    1204, // Lao
    1208, // Latvia
    1215, // Lebanon
    1223, // Lesotho
    1231, // Liberia
    1239, // LibyanArabJamahiriya
    1260, // Liechtenstein
    1274, // Lithuania
    1284, // Luxembourg
    1295, // Macau
    1301, // Macedonia
    1311, // Madagascar
    1322, // Malawi
    1329, // Malaysia
    1338, // Maldives
    1347, // Mali
    1352, // Malta
    1358, // MarshallIslands
    1374, // Martinique
    1385, // Mauritania
    1396, // Mauritius
    1406, // Mayotte
    1414, // Mexico
    1421, // Micronesia
    1432, // Moldova
    1440, // Monaco
    1447, // Mongolia
    1456, // Montserrat
    1467, // Morocco
    1475, // Mozambique
    1486, // Myanmar
    1494, // Namibia
    1502, // Nauru
    1508, // Nepal
    1514, // Netherlands
    1526, // NetherlandsAntilles
    1546, // NewCaledonia
    1559, // NewZealand
    1570, // Nicaragua
    1580, // Niger
    1586, // Nigeria
    1594, // Niue
    1599, // NorfolkIsland
    1613, // NorthernMarianaIslands
    1636, // Norway
    1643, // Oman
    1648, // Pakistan
    1657, // Palau
    1663, // PalestinianTerritory
    1684, // Panama
    1691, // PapuaNewGuinea
    1706, // Paraguay
    1715, // Peru
    1720, // Philippines
    1732, // Pitcairn
    1741, // Poland
    1748, // Portugal
    1757, // PuertoRico
    1768, // Qatar
    1774, // Reunion
    1782, // Romania
    1790, // RussianFederation
    1808, // Rwanda
    1815, // SaintKittsAndNevis
    1834, // StLucia
    1842, // StVincentAndTheGrenadines
    1868, // Samoa
    1874, // SanMarino
    1884, // SaoTomeAndPrincipe
    1903, // SaudiArabia
    1915, // Senegal
    1923, // Seychelles
    1934, // SierraLeone
    1946, // Singapore
    1956, // Slovakia
    1965, // Slovenia
    1974, // SolomonIslands
    1989, // Somalia
    1997, // SouthAfrica
    2009, // SouthGeorgiaAndTheSouthSandwichIslands
    2048, // Spain
    2054, // SriLanka
    2063, // StHelena
    2072, // StPierreAndMiquelon
    2092, // Sudan
    2098, // Suriname
    2107, // SvalbardAndJanMayenIslands
    2134, // Swaziland
    2144, // Sweden
    2151, // Switzerland
    2163, // SyrianArabRepublic
    2182, // Taiwan
    2189, // Tajikistan
    2200, // Tanzania
    2209, // Thailand
    2218, // Togo
    2223, // Tokelau
    2231, // Tonga
    2237, // TrinidadAndTobago
    2255, // Tunisia
    2263, // Turkey
    2270, // Turkmenistan
    2283, // TurksAndCaicosIslands
    2305, // Tuvalu
    2312, // Uganda
    2319, // Ukraine
    2327, // UnitedArabEmirates
    2346, // UnitedKingdom
    2360, // UnitedStates
    2373, // UnitedStatesMinorOutlyingIslands
    2406, // Uruguay
    2414, // Uzbekistan
    2425, // Vanuatu
    2433, // VaticanCityState
    2450, // Venezuela
    2460, // VietNam
    2468, // BritishVirginIslands
    2489, // USVirginIslands
    2505, // WallisAndFutunaIslands
    2528, // WesternSahara
    2542, // Yemen
    2548, // Yugoslavia
    2559, // Zambia
    2566, // Zimbabwe
    2575, // SerbiaAndMontenegro
    2595, // Montenegro
    2606, // Serbia
    2613, // Saint Barthelemy
    2630, // Saint Martin
    2643, // LatinAmericaAndTheCaribbean
};

// Assumes that code is a 2 letter code
DLLEXPORT QString fullCountryFromCode(const QString& countryCode);

}
}

#endif
