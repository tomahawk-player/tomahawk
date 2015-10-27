/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011,      Dominik Schmidt <domme@tomahawk-player.org>
 *   Copyright 2011-2012, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2011,      Thierry Goeckel
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013-2014, Uwe L. Korn <uwelk@xhochy.com>
 *   Copyright 2014,      Enno Gottschalk <mrmaffen@googlemail.com>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 */

// if run in phantomjs add fake Tomahawk environment
if ((typeof Tomahawk === "undefined") || (Tomahawk === null)) {
    var Tomahawk = {
        fakeEnv: function () {
            return true;
        },
        resolverData: function () {
            return {
                scriptPath: function () {
                    return "/home/tomahawk/resolver.js";
                }
            };
        },
        log: function () {
            console.log.apply(arguments);
        }
    };
}

Tomahawk.apiVersion = "0.2.2";



Tomahawk.dumpResult = function (result) {
    var results = result.results;
    Tomahawk.log("Dumping " + results.length + " results for query " + result.qid + "...");
    for (var i = 0; i < results.length; i++) {
        Tomahawk.log(results[i].artist + " - " + results[i].track + " | " + results[i].url);
    }

    Tomahawk.log("Done.");
};

// help functions

Tomahawk.valueForSubNode = function (node, tag) {
    if (node === undefined) {
        throw new Error("Tomahawk.valueForSubnode: node is undefined!");
    }

    var element = node.getElementsByTagName(tag)[0];
    if (element === undefined) {
        return undefined;
    }

    return element.textContent;
};


/**
 * Internal counter used to identify retrievedMetadata call back from native
 * code.
 */
Tomahawk.retrieveMetadataIdCounter = 0;
/**
 * Internal map used to map metadataIds to the respective JavaScript callbacks.
 */
Tomahawk.retrieveMetadataCallbacks = {};

/**
 * Retrieve metadata for a media stream.
 *
 * @param url String The URL which should be scanned for metadata.
 * @param mimetype String The mimetype of the stream, e.g. application/ogg
 * @param sizehint Size in bytes if not supplied possibly the whole file needs
 *          to be downloaded
 * @param options Object Map to specify various parameters related to the media
 *          URL. This includes:
 *          * headers: Object of HTTP(S) headers that should be set on doing the
 *                     request.
 *          * method: String HTTP verb to be used (default: GET)
 *          * username: Username when using authentication
 *          * password: Password when using authentication
 * @param callback Function(Object,String) This function is called on completeion.
 *          If an error occured, error is set to the corresponding message else
 *          null.
 */
Tomahawk.retrieveMetadata = function (url, mimetype, sizehint, options, callback) {
    var metadataId = Tomahawk.retrieveMetadataIdCounter;
    Tomahawk.retrieveMetadataIdCounter++;
    Tomahawk.retrieveMetadataCallbacks[metadataId] = callback;
    Tomahawk.nativeRetrieveMetadata(metadataId, url, mimetype, sizehint, options);
};

/**
 * Pass the natively retrieved metadata back to the JavaScript callback.
 *
 * Internal use only!
 */
Tomahawk.retrievedMetadata = function (metadataId, metadata, error) {
    // Check that we have a matching callback stored.
    if (!Tomahawk.retrieveMetadataCallbacks.hasOwnProperty(metadataId)) {
        return;
    }

    // Call the real callback
    if (Tomahawk.retrieveMetadataCallbacks.hasOwnProperty(metadataId)) {
        Tomahawk.retrieveMetadataCallbacks[metadataId](metadata, error);
    }

    // Callback are only used once.
    delete Tomahawk.retrieveMetadataCallbacks[metadataId];
};

/**
 * Internal counter used to identify asyncRequest callback from native code.
 */
Tomahawk.asyncRequestIdCounter = 0;
/**
 * Internal map used to map asyncRequestIds to the respective javascript
 * callback functions.
 */
Tomahawk.asyncRequestCallbacks = {};

/**
 * Pass the natively retrieved reply back to the javascript callback
 * and augment the fake XMLHttpRequest object.
 *
 * Internal use only!
 */
Tomahawk.nativeAsyncRequestDone = function (reqId, xhr) {
    // Check that we have a matching callback stored.
    if (!Tomahawk.asyncRequestCallbacks.hasOwnProperty(reqId)) {
        return;
    }

    // Call the real callback
    if (xhr.readyState == 4 && httpSuccessStatuses.indexOf(xhr.status) != -1) {
        // Call the real callback
        if (Tomahawk.asyncRequestCallbacks[reqId].callback) {
            Tomahawk.asyncRequestCallbacks[reqId].callback(xhr);
        }
    } else if (xhr.readyState === 4) {
        Tomahawk.log("Failed to do nativeAsyncRequest");
        Tomahawk.log("Status Code was: " + xhr.status);
        if (Tomahawk.asyncRequestCallbacks[reqId].errorHandler) {
            Tomahawk.asyncRequestCallbacks[reqId].errorHandler(xhr);
        }
    }

    // Callbacks are only used once.
    delete Tomahawk.asyncRequestCallbacks[reqId];
};



Tomahawk.assert = function (assertion, message) {
    Tomahawk.nativeAssert(assertion, message);
};

Tomahawk.sha256 = Tomahawk.sha256 || function (message) {
        return CryptoJS.SHA256(message).toString(CryptoJS.enc.Hex);
    };
Tomahawk.md5 = Tomahawk.md5 || function (message) {
        return CryptoJS.MD5(message).toString(CryptoJS.enc.Hex);
    };
// Return a HMAC (md5) signature of the input text with the desired key
Tomahawk.hmac = function (key, message) {
    return CryptoJS.HmacMD5(message, key).toString(CryptoJS.enc.Hex);
};

Tomahawk.localStorage = Tomahawk.localStorage || {
        setItem: function (key, value) {
            window.localStorage[key] = value;
        },
        getItem: function (key) {
            return window.localStorage[key];
        },
        removeItem: function (key) {
            delete window.localStorage[key];
        }
    };

// some aliases
Tomahawk.setTimeout = Tomahawk.setTimeout || window.setTimeout;
Tomahawk.setInterval = Tomahawk.setInterval || window.setInterval;
Tomahawk.base64Decode = function (a) {
    return window.atob(a);
};
Tomahawk.base64Encode = function (b) {
    return window.btoa(b);
};

Tomahawk.NativeScriptJobManager = {
    idCounter: 0,
    deferreds: {},
    invoke: function (methodName, params) {
        var requestId = this.idCounter++;
        Tomahawk.invokeNativeScriptJob(requestId, methodName, JSON.stringify(params));
        this.deferreds[requestId] = RSVP.defer();
        return this.deferreds[requestId].promise;
    },
    reportNativeScriptJobResult: function (requestId, result) {
        var deferred = this.deferreds[requestId];
        if (!deferred) {
            Tomahawk.log("Deferred object with the given requestId is not present!");
        }
        deferred.resolve(result);
    }
};
<<<<<<< HEAD

Tomahawk.ConfigTestResultType = {
    Other: 0,
    Success: 1,
    Logout: 2,
    CommunicationError: 3,
    InvalidCredentials: 4,
    InvalidAccount: 5,
    PlayingElsewhere: 6,
    AccountExpired: 7
};

Tomahawk.Country = {
    AnyCountry: 0,
    Afghanistan: 1,
    Albania: 2,
    Algeria: 3,
    AmericanSamoa: 4,
    Andorra: 5,
    Angola: 6,
    Anguilla: 7,
    Antarctica: 8,
    AntiguaAndBarbuda: 9,
    Argentina: 10,
    Armenia: 11,
    Aruba: 12,
    Australia: 13,
    Austria: 14,
    Azerbaijan: 15,
    Bahamas: 16,
    Bahrain: 17,
    Bangladesh: 18,
    Barbados: 19,
    Belarus: 20,
    Belgium: 21,
    Belize: 22,
    Benin: 23,
    Bermuda: 24,
    Bhutan: 25,
    Bolivia: 26,
    BosniaAndHerzegowina: 27,
    Botswana: 28,
    BouvetIsland: 29,
    Brazil: 30,
    BritishIndianOceanTerritory: 31,
    BruneiDarussalam: 32,
    Bulgaria: 33,
    BurkinaFaso: 34,
    Burundi: 35,
    Cambodia: 36,
    Cameroon: 37,
    Canada: 38,
    CapeVerde: 39,
    CaymanIslands: 40,
    CentralAfricanRepublic: 41,
    Chad: 42,
    Chile: 43,
    China: 44,
    ChristmasIsland: 45,
    CocosIslands: 46,
    Colombia: 47,
    Comoros: 48,
    DemocraticRepublicOfCongo: 49,
    PeoplesRepublicOfCongo: 50,
    CookIslands: 51,
    CostaRica: 52,
    IvoryCoast: 53,
    Croatia: 54,
    Cuba: 55,
    Cyprus: 56,
    CzechRepublic: 57,
    Denmark: 58,
    Djibouti: 59,
    Dominica: 60,
    DominicanRepublic: 61,
    EastTimor: 62,
    Ecuador: 63,
    Egypt: 64,
    ElSalvador: 65,
    EquatorialGuinea: 66,
    Eritrea: 67,
    Estonia: 68,
    Ethiopia: 69,
    FalklandIslands: 70,
    FaroeIslands: 71,
    FijiCountry: 72,
    Finland: 73,
    France: 74,
    MetropolitanFrance: 75,
    FrenchGuiana: 76,
    FrenchPolynesia: 77,
    FrenchSouthernTerritories: 78,
    Gabon: 79,
    Gambia: 80,
    Georgia: 81,
    Germany: 82,
    Ghana: 83,
    Gibraltar: 84,
    Greece: 85,
    Greenland: 86,
    Grenada: 87,
    Guadeloupe: 88,
    Guam: 89,
    Guatemala: 90,
    Guinea: 91,
    GuineaBissau: 92,
    Guyana: 93,
    Haiti: 94,
    HeardAndMcDonaldIslands: 95,
    Honduras: 96,
    HongKong: 97,
    Hungary: 98,
    Iceland: 99,
    India: 100,
    Indonesia: 101,
    Iran: 102,
    Iraq: 103,
    Ireland: 104,
    Israel: 105,
    Italy: 106,
    Jamaica: 107,
    Japan: 108,
    Jordan: 109,
    Kazakhstan: 110,
    Kenya: 111,
    Kiribati: 112,
    DemocraticRepublicOfKorea: 113,
    RepublicOfKorea: 114,
    Kuwait: 115,
    Kyrgyzstan: 116,
    Lao: 117,
    Latvia: 118,
    Lebanon: 119,
    Lesotho: 120,
    Liberia: 121,
    LibyanArabJamahiriya: 122,
    Liechtenstein: 123,
    Lithuania: 124,
    Luxembourg: 125,
    Macau: 126,
    Macedonia: 127,
    Madagascar: 128,
    Malawi: 129,
    Malaysia: 130,
    Maldives: 131,
    Mali: 132,
    Malta: 133,
    MarshallIslands: 134,
    Martinique: 135,
    Mauritania: 136,
    Mauritius: 137,
    Mayotte: 138,
    Mexico: 139,
    Micronesia: 140,
    Moldova: 141,
    Monaco: 142,
    Mongolia: 143,
    Montserrat: 144,
    Morocco: 145,
    Mozambique: 146,
    Myanmar: 147,
    Namibia: 148,
    NauruCountry: 149,
    Nepal: 150,
    Netherlands: 151,
    NetherlandsAntilles: 152,
    NewCaledonia: 153,
    NewZealand: 154,
    Nicaragua: 155,
    Niger: 156,
    Nigeria: 157,
    Niue: 158,
    NorfolkIsland: 159,
    NorthernMarianaIslands: 160,
    Norway: 161,
    Oman: 162,
    Pakistan: 163,
    Palau: 164,
    PalestinianTerritory: 165,
    Panama: 166,
    PapuaNewGuinea: 167,
    Paraguay: 168,
    Peru: 169,
    Philippines: 170,
    Pitcairn: 171,
    Poland: 172,
    Portugal: 173,
    PuertoRico: 174,
    Qatar: 175,
    Reunion: 176,
    Romania: 177,
    RussianFederation: 178,
    Rwanda: 179,
    SaintKittsAndNevis: 180,
    StLucia: 181,
    StVincentAndTheGrenadines: 182,
    Samoa: 183,
    SanMarino: 184,
    SaoTomeAndPrincipe: 185,
    SaudiArabia: 186,
    Senegal: 187,
    SerbiaAndMontenegro: 241,
    Seychelles: 188,
    SierraLeone: 189,
    Singapore: 190,
    Slovakia: 191,
    Slovenia: 192,
    SolomonIslands: 193,
    Somalia: 194,
    SouthAfrica: 195,
    SouthGeorgiaAndTheSouthSandwichIslands: 196,
    Spain: 197,
    SriLanka: 198,
    StHelena: 199,
    StPierreAndMiquelon: 200,
    Sudan: 201,
    Suriname: 202,
    SvalbardAndJanMayenIslands: 203,
    Swaziland: 204,
    Sweden: 205,
    Switzerland: 206,
    SyrianArabRepublic: 207,
    Taiwan: 208,
    Tajikistan: 209,
    Tanzania: 210,
    Thailand: 211,
    Togo: 212,
    Tokelau: 213,
    TongaCountry: 214,
    TrinidadAndTobago: 215,
    Tunisia: 216,
    Turkey: 217,
    Turkmenistan: 218,
    TurksAndCaicosIslands: 219,
    Tuvalu: 220,
    Uganda: 221,
    Ukraine: 222,
    UnitedArabEmirates: 223,
    UnitedKingdom: 224,
    UnitedStates: 225,
    UnitedStatesMinorOutlyingIslands: 226,
    Uruguay: 227,
    Uzbekistan: 228,
    Vanuatu: 229,
    VaticanCityState: 230,
    Venezuela: 231,
    VietNam: 232,
    BritishVirginIslands: 233,
    USVirginIslands: 234,
    WallisAndFutunaIslands: 235,
    WesternSahara: 236,
    Yemen: 237,
    Yugoslavia: 238,
    Zambia: 239,
    Zimbabwe: 240,
    Montenegro: 242,
    Serbia: 243,
    SaintBarthelemy: 244,
    SaintMartin: 245,
    LatinAmericaAndTheCaribbean: 246
};

Tomahawk.Collection = {
    BrowseCapability: {
        Artists: 1,
        Albums: 2,
        Tracks: 4
    },

    cachedDbs: {},

    Transaction: function (collection, id) {

        this.ensureDb = function () {
            return new RSVP.Promise(function (resolve, reject) {
                if (!collection.cachedDbs.hasOwnProperty(id)) {
                    Tomahawk.log("Opening database");
                    var estimatedSize = 5 * 1024 * 1024; // 5MB
                    collection.cachedDbs[id] =
                        openDatabase(id + "_collection", "", "Collection", estimatedSize);

                    collection.cachedDbs[id].transaction(function (tx) {
                        Tomahawk.log("Creating initial db tables");
                        tx.executeSql("CREATE TABLE IF NOT EXISTS artists(" +
                            "_id INTEGER PRIMARY KEY AUTOINCREMENT," +
                            "artist TEXT ," +
                            "artistDisambiguation TEXT," +
                            "UNIQUE (artist, artistDisambiguation) ON CONFLICT IGNORE)", []);
                        tx.executeSql("CREATE TABLE IF NOT EXISTS albumArtists(" +
                            "_id INTEGER PRIMARY KEY AUTOINCREMENT," +
                            "albumArtist TEXT ," +
                            "albumArtistDisambiguation TEXT," +
                            "UNIQUE (albumArtist, albumArtistDisambiguation) ON CONFLICT IGNORE)",
                            []);
                        tx.executeSql("CREATE TABLE IF NOT EXISTS albums(" +
                            "_id INTEGER PRIMARY KEY AUTOINCREMENT," +
                            "album TEXT," +
                            "albumArtistId INTEGER," +
                            "UNIQUE (album, albumArtistId) ON CONFLICT IGNORE," +
                            "FOREIGN KEY(albumArtistId) REFERENCES albumArtists(_id))", []);
                        tx.executeSql("CREATE TABLE IF NOT EXISTS artistAlbums(" +
                            "_id INTEGER PRIMARY KEY AUTOINCREMENT," +
                            "albumId INTEGER," +
                            "artistId INTEGER," +
                            "UNIQUE (albumId, artistId) ON CONFLICT IGNORE," +
                            "FOREIGN KEY(albumId) REFERENCES albums(_id)," +
                            "FOREIGN KEY(artistId) REFERENCES artists(_id))", []);
                        tx.executeSql("CREATE TABLE IF NOT EXISTS tracks(" +
                            "_id INTEGER PRIMARY KEY AUTOINCREMENT," +
                            "track TEXT," +
                            "artistId INTEGER," +
                            "albumId INTEGER," +
                            "url TEXT," +
                            "duration INTEGER," +
                            "albumPos INTEGER," +
                            "linkUrl TEXT," +
                            'releaseyear INTEGER,' +
                            'bitrate INTEGER,' +
                            "UNIQUE (track, artistId, albumId) ON CONFLICT IGNORE," +
                            "FOREIGN KEY(artistId) REFERENCES artists(_id)," +
                            "FOREIGN KEY(albumId) REFERENCES albums(_id))", []);
                    });
                }
                resolve(collection.cachedDbs[id]);
            });
        };

        this.beginTransaction = function () {
            var that = this;
            return this.ensureDb().then(function (db) {
                return new RSVP.Promise(function (resolve, reject) {
                    that.db = db;
                    that.statements = [];
                    resolve();
                })
            });
        };

        this.execDeferredStatements = function (resolve, reject) {
            var that = this;
            that.stmtsToResolve = that.statements.length;
            that.results = that.statements.slice();
            Tomahawk.log('Executing ' + that.stmtsToResolve
                + ' deferred SQL statements in transaction');
            return new RSVP.Promise(function (resolve, reject) {
                if (that.statements.length == 0) {
                    resolve([]);
                } else {
                    that.db.transaction(function (tx) {
                        for (var i = 0; i < that.statements.length; ++i) {
                            var stmt = that.statements[i];
                            tx.executeSql(stmt.statement, stmt.args,
                                (function () {
                                    //A function returning a function to
                                    //capture value of i
                                    var originalI = i;
                                    return function (tx, results) {
                                        if (typeof that.statements[originalI].map !== 'undefined') {
                                            var map = that.statements[originalI].map;
                                            that.results[originalI] = [];
                                            for (var ii = 0; ii < results.rows.length; ii++) {
                                                that.results[originalI].push(map(
                                                    results.rows.item(ii)
                                                ));
                                            }
                                        }
                                        else {
                                            that.results[originalI] = results;
                                        }
                                        that.stmtsToResolve--;
                                        if (that.stmtsToResolve == 0) {
                                            that.statements = [];
                                            resolve(that.results);
                                        }
                                    };
                                })(), function (tx, error) {
                                    Tomahawk.log("Error in tx.executeSql: " + error.code + " - "
                                        + error.message);
                                    that.statements = [];
                                    reject(error);
                                }
                            );
                        }
                    });
                }
            });
        };

        this.sql = function (sqlStatement, sqlArgs, mapFunction) {
            this.statements.push({statement: sqlStatement, args: sqlArgs, map: mapFunction});
        };

        this.sqlSelect = function (table, mapResults, fields, where, join) {
            var whereKeys = [];
            var whereValues = [];
            for (var whereKey in where) {
                if (where.hasOwnProperty(whereKey)) {
                    whereKeys.push(table + "." + whereKey + " = ?");
                    whereValues.push(where[whereKey]);
                }
            }
            var whereString = whereKeys.length > 0 ? " WHERE " + whereKeys.join(" AND ") : "";

            var joinString = "";
            for (var i = 0; join && i < join.length; i++) {
                var joinConditions = [];
                for (var joinKey in join[i].conditions) {
                    if (join[i].conditions.hasOwnProperty(joinKey)) {
                        joinConditions.push(table + "." + joinKey + " = " + join[i].table + "."
                            + join[i].conditions[joinKey]);
                    }
                }
                joinString += " INNER JOIN " + join[i].table + " ON "
                    + joinConditions.join(" AND ");
            }

            var fieldsString = fields && fields.length > 0 ? fields.join(", ") : "*";
            var statement = "SELECT " + fieldsString + " FROM " + table + joinString + whereString;
            return this.sql(statement, whereValues, mapResults);
        };

        this.sqlInsert = function (table, fields) {
            var fieldsKeys = [];
            var fieldsValues = [];
            var valuesString = "";
            for (var key in fields) {
                fieldsKeys.push(key);
                fieldsValues.push(fields[key]);
                if (valuesString.length > 0) {
                    valuesString += ", ";
                }
                valuesString += "?";
            }
            var statement = "INSERT INTO " + table + " (" + fieldsKeys.join(", ") + ") VALUES ("
                + valuesString + ")";
            return this.sql(statement, fieldsValues);
        };

        this.sqlDrop = function (table) {
            var statement = "DROP TABLE IF EXISTS " + table;
            return this.sql(statement, []);
        };

    },

    addTracks: function (params) {
        var that = this;
        var id = params.id;
        var tracks = params.tracks;

        var cachedAlbumArtists = {},
            cachedArtists = {},
            cachedAlbums = {},
            cachedArtistIds = {},
            cachedAlbumIds = {};

        var t = new Tomahawk.Collection.Transaction(this, id);
        return t.beginTransaction().then(function () {
            // First we insert all artists and albumArtists
            t.sqlInsert("artists", {
                artist: "Various Artists",
                artistDisambiguation: ""
            });
            for (var i = 0; i < tracks.length; i++) {
                tracks[i].track = tracks[i].track || "";
                tracks[i].album = tracks[i].album || "";
                tracks[i].artist = tracks[i].artist || "";
                tracks[i].artistDisambiguation = tracks[i].artistDisambiguation || "";
                tracks[i].albumArtist = tracks[i].albumArtist || "";
                tracks[i].albumArtistDisambiguation = tracks[i].albumArtistDisambiguation || "";
                (function (track) {
                    t.sqlInsert("artists", {
                        artist: track.artist,
                        artistDisambiguation: track.artistDisambiguation
                    });
                    t.sqlInsert("albumArtists", {
                        albumArtist: track.albumArtist,
                        albumArtistDisambiguation: track.albumArtistDisambiguation
                    });
                })(tracks[i]);
            }
            return t.execDeferredStatements();
        }).then(function () {
            // Get all artists' and albumArtists' db ids
            t.sqlSelect("albumArtists", function (r) {
                return {
                    albumArtist: r.albumArtist,
                    albumArtistDisambiguation: r.albumArtistDisambiguation,
                    _id: r._id
                };
            });
            t.sqlSelect("artists", function (r) {
                return {
                    artist: r.artist,
                    artistDisambiguation: r.artistDisambiguation,
                    _id: r._id
                };
            });
            return t.execDeferredStatements();
        }).then(function (resultsArray) {
            // Store the db ids in a map
            var i, row, albumArtists = {};
            for (i = 0; i < resultsArray[0].length; i++) {
                row = resultsArray[0][i];
                albumArtists[row.albumArtist + "♣" + row.albumArtistDisambiguation] = row._id;
            }
            for (i = 0; i < resultsArray[1].length; i++) {
                row = resultsArray[1][i];
                cachedArtists[row.artist + "♣" + row.artistDisambiguation] = row._id;
                cachedArtistIds[row._id] = {
                    artist: row.artist,
                    artistDisambiguation: row.artistDisambiguation
                };
            }

            for (i = 0; i < tracks.length; i++) {
                var track = tracks[i];
                var album = cachedAlbumArtists[track.album];
                if (!album) {
                    album = cachedAlbumArtists[track.album] = {
                        artists: {}
                    };
                }
                album.artists[track.artist] = true;
                var artistCount = Object.keys(album.artists).length;
                if (artistCount == 1) {
                    album.albumArtistId =
                        cachedArtists[track.artist + "♣" + track.artistDisambiguation];
                } else if (artistCount == 2) {
                    album.albumArtistId = cachedArtists["Various Artists♣"];
                }
            }
        }).then(function () {
            // Insert all albums
            for (var i = 0; i < tracks.length; i++) {
                (function (track) {
                    var albumArtistId = cachedAlbumArtists[track.album].albumArtistId;
                    t.sqlInsert("albums", {
                        album: track.album,
                        albumArtistId: albumArtistId
                    });
                })(tracks[i]);
            }
            return t.execDeferredStatements();
        }).then(function () {
            // Get the albums' db ids
            t.sqlSelect("albums", function (r) {
                return {
                    album: r.album,
                    albumArtistId: r.albumArtistId,
                    _id: r._id
                };
            });
            return t.execDeferredStatements();
        }).then(function (results) {
            // Store the db ids in a map
            results = results[0];
            for (var i = 0; i < results.length; i++) {
                var row = results[i];
                cachedAlbums[row.album + "♣" + row.albumArtistId] = row._id;
                cachedAlbumIds[row._id] = {
                    album: row.album,
                    albumArtistId: row.albumArtistId
                };
            }
        }).then(function () {
            // Now we are ready to insert the tracks
            for (var i = 0; i < tracks.length; i++) {
                (function (track) {
                    // Get all relevant ids that we stored in the previous steps
                    var artistId = cachedArtists[track.artist + "♣" + track.artistDisambiguation];
                    var albumArtistId = cachedAlbumArtists[track.album].albumArtistId;
                    var albumId = cachedAlbums[track.album + "♣" + albumArtistId];
                    // Insert the artist <=> album relations
                    t.sqlInsert("artistAlbums", {
                        albumId: albumId,
                        artistId: artistId
                    });
                    // Insert the tracks
                    t.sqlInsert("tracks", {
                        track: track.track,
                        artistId: artistId,
                        albumId: albumId,
                        url: track.url,
                        duration: track.duration,
                        linkUrl: track.linkUrl,
                        releaseyear: track.releaseyear,
                        bitrate: track.bitrate,
                        albumPos: track.albumpos
                    });
                })(tracks[i]);
            }
            return t.execDeferredStatements();
        }).then(function () {
            var resultMap = function (r) {
                return {
                    _id: r._id,
                    artistId: r.artistId,
                    albumId: r.albumId,
                    track: r.track
                };
            };
            // Get the tracks' db ids
            t.sqlSelect("tracks", resultMap, ["_id", "artistId", "albumId", "track"]);
            return t.execDeferredStatements();
        }).then(function (results) {
            that._trackCount = results[0].length;
            Tomahawk.log("Added " + results[0].length + " tracks to collection '" + id + "'");
            // Add the db ids together with the basic metadata to the fuzzy index list
            var fuzzyIndexList = [];
            for (var i = 0; i < results[0].length; i++) {
                var row = results[0][i];
                fuzzyIndexList.push({
                    id: row._id,
                    artist: cachedArtistIds[row.artistId].artist,
                    album: cachedAlbumIds[row.albumId].album,
                    track: row.track
                });
            }
            Tomahawk.createFuzzyIndex(fuzzyIndexList);
        });
    },

    wipe: function (params) {
        var id = params.id;

        var that = this;

        var t = new Tomahawk.Collection.Transaction(this, id);
        return t.beginTransaction().then(function () {
            t.sqlDrop("artists");
            t.sqlDrop("albumArtists");
            t.sqlDrop("albums");
            t.sqlDrop("artistAlbums");
            t.sqlDrop("tracks");
            return t.execDeferredStatements();
        }).then(function () {
            return new RSVP.Promise(function (resolve, reject) {
                that.cachedDbs[id].changeVersion(that.cachedDbs[id].version, "", null,
                    function (err) {
                        if (console.error) {
                            console.error("Error!: %o", err);
                        }
                        reject();
                    }, function () {
                        delete that.cachedDbs[id];
                        Tomahawk.deleteFuzzyIndex(id);
                        Tomahawk.log("Wiped collection '" + id + "'");
                        resolve();
                    });
            });
        });
    },

    _fuzzyIndexIdsToTracks: function (resultIds, id) {
        if (typeof id === 'undefined') {
            id = this.settings.id;
        }
        var t = new Tomahawk.Collection.Transaction(this, id);
        return t.beginTransaction().then(function () {
            var mapFn = function (row) {
                return {
                    artist: row.artist,
                    artistDisambiguation: row.artistDisambiguation,
                    album: row.album,
                    track: row.track,
                    duration: row.duration,
                    url: row.url,
                    linkUrl: row.linkUrl,
                    releaseyear: row.releaseyear,
                    bitrate: row.bitrate,
                    albumpos: row.albumPos
                };
            };
            for (var idx = 0; resultIds && idx < resultIds.length; idx++) {
                var trackid = resultIds[idx][0];
                var where = {_id: trackid};
                t.sqlSelect("tracks", mapFn,
                    [],
                    where, [
                        {
                            table: "artists",
                            conditions: {
                                artistId: "_id"
                            }
                        }, {
                            table: "albums",
                            conditions: {
                                albumId: "_id"
                            }
                        }
                    ]
                );
            }
            return t.execDeferredStatements();
        }).then(function (results) {
            var merged = [];
            return merged.concat.apply(merged,
                results.map(function (e) {
                    //every result has one track
                    return e[0];
                }));
        });
    },

    resolve: function (params) {
        var resultIds = Tomahawk.resolveFromFuzzyIndex(params.artist, params.album, params.track);
        return this._fuzzyIndexIdsToTracks(resultIds);
    },

    search: function (params) {
        var resultIds = Tomahawk.searchFuzzyIndex(params.query);
        return this._fuzzyIndexIdsToTracks(resultIds);
    },

    tracks: function (params, where) {
        //TODO filter/where support
        var id = params.id;
        if (typeof id === 'undefined') {
            id = this.settings.id;
        }

        var t = new Tomahawk.Collection.Transaction(this, id);
        return t.beginTransaction().then(function () {
            var mapFn = function (row) {
                return {
                    artist: row.artist,
                    artistDisambiguation: row.artistDisambiguation,
                    album: row.album,
                    track: row.track,
                    duration: row.duration,
                    url: row.url,
                    linkUrl: row.linkUrl,
                    releaseyear: row.releaseyear,
                    bitrate: row.bitrate,
                    albumpos: row.albumPos
                };
            };
            t.sqlSelect("tracks", mapFn,
                [],
                where, [
                    {
                        table: "artists",
                        conditions: {
                            artistId: "_id"
                        }
                    }, {
                        table: "albums",
                        conditions: {
                            albumId: "_id"
                        }
                    }
                ]
            );
            return t.execDeferredStatements();
        }).then(function (results) {
            return {results: resolverInstance._convertUrls(results[0])};
        });
    },

    albums: function (params, where) {
        //TODO filter/where support
        var id = params.id;
        if (typeof id === 'undefined') {
            id = this.settings.id;
        }

        var t = new Tomahawk.Collection.Transaction(this, id);
        return t.beginTransaction().then(function () {
            var mapFn = function (row) {
                return {
                    albumArtist: row.artist,
                    albumArtistDisambiguation: row.artistDisambiguation,
                    album: row.album
                };
            };
            t.sqlSelect("albums", mapFn,
                ["album", "artist", "artistDisambiguation"],
                where, [
                    {
                        table: "artists",
                        conditions: {
                            albumArtistId: "_id"
                        }
                    }
                ]
            );
            return t.execDeferredStatements();
        }).then(function (results) {
            results = results[0].filter(function (e) {
                return (e.albumArtist != '' && e.album != '');
            });
            return {
                artists: results.map(function (i) {
                    return i.albumArtist;
                }),
                albums: results.map(function (i) {
                    return i.album;
                })
            };
        });
    },

    artists: function (params) {
        //TODO filter/where support
        var id = params.id;
        if (typeof id === 'undefined') {
            id = this.settings.id;
        }

        var t = new Tomahawk.Collection.Transaction(this, id);
        return t.beginTransaction().then(function () {
            var mapFn = function (r) {
                return r.artist;
            };
            t.sqlSelect("artists", mapFn, ["artist", "artistDisambiguation"]);
            return t.execDeferredStatements();
        }).then(function (artists) {
            return {artists: artists[0]};
        });
    },

    //TODO: not exactly sure how is this one supposed to work
    //albumArtists: function (params) {
    //var id = params.id;

    //var t = new Tomahawk.Collection.Transaction(this, id);
    //return t.beginTransaction().then(function () {
    //var mapFn = function(row) {
    //return {
    //albumArtist: row.albumArtist,
    //albumArtistDisambiguation: row.albumArtistDisambiguation
    //};
    //};
    //t.sqlSelect("albumArtists", ["albumArtist", "albumArtistDisambiguation"]);
    //return t.execDeferredStatements();
    //}).then(function (results) {
    //return results[0];
    //});
    //},

    artistAlbums: function (params) {
        //TODO filter/where support
        var id = params.id;
        if (typeof id === 'undefined') {
            id = this.settings.id;
        }
        var artist = params.artist;
        //var artistDisambiguation = params.artistDisambiguation;

        var t = new Tomahawk.Collection.Transaction(this, id);
        return t.beginTransaction().then(function () {

            t.sqlSelect("artists", function (r) {
                return r._id;
            }, ["_id"], {
                artist: artist
                //artistDisambiguation: artistDisambiguation
            });
            return t.execDeferredStatements();
        }).then(function (results) {
            var artistId = results[0][0];
            t.sqlSelect("artistAlbums", function (r) {
                return r.album;
            }, ["albumId", 'album'], {
                artistId: artistId
            }, [
                {
                    table: "albums",
                    conditions: {
                        albumId: "_id"
                    }
                }
            ]);
            return t.execDeferredStatements();
        }).then(function (results) {
            return {
                artist: artist,
                albums: results[0]
            };
        });
    },

    albumTracks: function (params) {
        //TODO filter/where support
        var id = params.id;
        if (typeof id === 'undefined') {
            id = this.settings.id;
        }
        var albumArtist = params.artist;
        //var albumArtistDisambiguation = params.albumArtistDisambiguation;
        var album = params.album;

        var that = this;

        var t = new Tomahawk.Collection.Transaction(this, id);
        return t.beginTransaction().then(function () {
            t.sqlSelect("artists", function (r) {
                return r._id;
            }, ["_id"], {
                artist: albumArtist
                //artistDisambiguation: albumArtistDisambiguation
            });
            return t.execDeferredStatements();
        }).then(function (results) {
            var albumArtistId = results[0][0];
            t.sqlSelect("albums", function (r) {
                return r._id;
            }, ["_id"], {
                album: album,
                albumArtistId: albumArtistId
            });
            return t.execDeferredStatements();
        }).then(function (results) {
            var albumId = results[0][0];
            return that.tracks(params, {
                albumId: albumId
            });
        });
    },

    collection: function () {
        this.settings.trackcount = this._trackCount;
        if (!this.settings.description) {
            this.settings.description = this.settings.prettyname;
        }
        this.settings.capabilities = [Tomahawk.Collection.BrowseCapability.Artists,
            Tomahawk.Collection.BrowseCapability.Albums,
            Tomahawk.Collection.BrowseCapability.Tracks];
        return this.settings;
    }
};

// Legacy compability for 0.8 and before
Tomahawk.reportCapabilities = function (capabilities) {
    if (capabilities & TomahawkResolverCapability.Browsable) {
        Tomahawk.PluginManager.registerPlugin("collection", Tomahawk.resolver.instance);
    }

    Tomahawk.nativeReportCapabilities(capabilities);
};

Tomahawk.addArtistResults = Tomahawk.addAlbumResults = Tomahawk.addAlbumTrackResults
    = function (result) {
    Tomahawk.PluginManager.resolve[result.qid](result);
    delete Tomahawk.PluginManager.resolve[result.qid];
};

Tomahawk.addTrackResults = function (result) {
    Tomahawk.PluginManager.resolve[result.qid](result.results);
    delete Tomahawk.PluginManager.resolve[result.qid];
};

Tomahawk.reportStreamUrl = function (qid, streamUrl, headers) {
    Tomahawk.PluginManager.resolve[qid]({
        url: streamUrl,
        headers: headers
    });
    delete Tomahawk.PluginManager.resolve[qid];
};

Tomahawk.addUrlResult = function (url, result) {
    /* Merge the whole mess into one consistent result which is independent of type
     var cleanResult = {
     type: result.type,
     guid: result.guid,
     info: result.info,
     creator: result.creator,
     linkUrl: result.url
     };
     if (cleanResult.type == "track") {
     cleanResult.track = result.title;
     cleanResult.artist = result.artist;
     } else if (cleanResult.type == "artist") {
     cleanResult.artist = result.name;
     } else if (cleanResult.type == "album") {
     cleanResult.album = result.name;
     cleanResult.artist = result.artist;
     } else if (cleanResult.type == "playlist") {
     cleanResult.title = result.title;
     } else if (cleanResult.type == "xspf-url") {
     cleanResult.url = result.url;
     }
     if (result.tracks) {
     cleanResult.tracks = [];
     var i;
     for (i=0;i<result.tracks.length;i++) {
     var cleanTrack = {
     track: result.tracks[i].title,
     artist: result.tracks[i].artist
     };
     cleanResult.push(cleanTrack)
     }
     Tomahawk.PluginManager.resolve[url](cleanResult);
     */
    Tomahawk.PluginManager.resolve[url](result);
    delete Tomahawk.PluginManager.resolve[url];
};
=======
>>>>>>> Move moar stuff over to es6 foo
