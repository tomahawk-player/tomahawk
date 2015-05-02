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

// install RSVP.Promise as global Promise
if(window.Promise === undefined) {
    window.Promise = window.RSVP.Promise;
    window.RSVP.on('error', function(reason) {
        if (reason) {
            console.error(reason.message, reason);
        } else {
            console.error('Error: error thrown from RSVP but it was empty');
        }
    });
}

/**
 * Compares versions strings
 * (version1 < version2) == -1
 * (version1 = version2) == 0
 * (version1 > version2) == 1
 */
Tomahawk.versionCompare = function (version1, version2) {
    var v1 = version1.split('.').map(function (item) { return parseInt(item); });
    var v2 = version2.split('.').map(function (item) { return parseInt(item); });
    var length = Math.max(v1.length, v2.length);
    var i = 0;

    for (; i < length; i++) {
        if (typeof v1[i] == "undefined" || v1[i] === null) {
            if (typeof v2[i] == "undefined" || v2[i] === null) {
                // v1 == v2
                return 0;
            } else if (v2[i] === 0) {
                continue;
            } else {
                // v1 < v2
                return -1;
            }
        } else if (typeof v2[i] == "undefined" || v2[i] === null) {
            if ( v1[i] === 0 ) {
                continue;
            } else {
                // v1 > v2
                return 1;
            }
        } else if (v2[i] > v1[i]) {
            // v1 < v2
            return -1;
        } else if (v2[i] < v1[i]) {
            // v1 > v2
            return 1;
        }
    }
    // v1 == v2
    return 0;
};

/**
 * Check if this is at least specified tomahawk-api-version.
 */
Tomahawk.atLeastVersion = function (version) {
    return (Tomahawk.versionCompare(Tomahawk.apiVersion, version) >= 0);
};


Tomahawk.resolver = {
    scriptPath: Tomahawk.resolverData().scriptPath
};

Tomahawk.timestamp = function () {
    return Math.round(new Date() / 1000);
};

Tomahawk.dumpResult = function (result) {
    var results = result.results,
        i = 0;
    Tomahawk.log("Dumping " + results.length + " results for query " + result.qid + "...");
    for (i = 0; i < results.length; i++) {
        Tomahawk.log(results[i].artist + " - " + results[i].track + " | " + results[i].url);
    }

    Tomahawk.log("Done.");
};

// javascript part of Tomahawk-Object API
Tomahawk.extend = function (object, members) {
    var F = function () {};
    F.prototype = object;
    var newObject = new F();

    for (var key in members) {
        newObject[key] = members[key];
    }

    return newObject;
};


var TomahawkResolverCapability = {
    NullCapability: 0,
    Browsable:      1,
    PlaylistSync:   2,
    AccountFactory: 4,
    UrlLookup:      8
};

var TomahawkUrlType = {
    Any: 0,
    Playlist: 1,
    Track: 2,
    Album: 4,
    Artist: 8
};

var TomahawkConfigTestResultType = {
    Other: 0,
    Success: 1,
    Logout: 2,
    CommunicationError: 3,
    InvalidCredentials: 4,
    InvalidAccount: 5,
    PlayingElsewhere: 6,
    AccountExpired: 7
};

/**
 * Resolver BaseObject, inherit it to implement your own resolver.
 */
var TomahawkResolver = {
    init: function() {
    },
    scriptPath: function () {
        return Tomahawk.resolverData().scriptPath;
    },
    getConfigUi: function () {
        return {};
    },
    getUserConfig: function () {
        return JSON.parse(window.localStorage[this.scriptPath()] || "{}");
    },
    saveUserConfig: function () {
        var configJson = JSON.stringify(Tomahawk.resolverData().config);
        window.localStorage[ this.scriptPath() ] = configJson;
        this.newConfigSaved();
    },
    newConfigSaved: function () {
    },
    resolve: function (qid, artist, album, title) {
        return {
            qid: qid
        };
    },
    search: function (qid, searchString) {
        return this.resolve( qid, "", "", searchString );
    },
    artists: function (qid) {
        return {
            qid: qid
        };
    },
    albums: function (qid, artist) {
        return {
            qid: qid
        };
    },
    tracks: function (qid, artist, album) {
        return {
            qid: qid
        };
    },
    collection: function () {
        return {};
    },
    _testConfig: function (config) {
        return Promise.resolve(this.testConfig(config)).then(function() {
            return { result: Tomahawk.ConfigTestResultType.Success };
        });
    },
    testConfig: function () {
    }
};

Tomahawk.Resolver = {};
Tomahawk.Resolver.Promise = Tomahawk.extend(TomahawkResolver, {
    _adapter_resolve: function (qid, artist, album, title) {
        Promise.resolve(this.resolve(artist, album, title)).then(function(results){
            Tomahawk.addTrackResults({
                'qid': qid,
                'results': results
            });
        });
    },

    _adapter_search: function (qid, query)
    {
        Promise.resolve(this.search(query)).then(function(results){
            Tomahawk.addTrackResults({
                'qid': qid,
                'results': results
            });
        });
    }
});

/**** begin example implementation of a resolver ****/


// implement the resolver
/*
 *    var DemoResolver = Tomahawk.extend(TomahawkResolver,
 *    {
 *        getSettings: function()
 *        {
 *            return {
 *                name: "Demo Resolver",
 *                weigth: 95,
 *                timeout: 5,
 *                limit: 10
 };
 },
 resolve: function( qid, artist, album, track )
 {
     return {
         qid: qid,
         results: [
         {
             artist: "Mokele",
             album: "You Yourself are Me Myself and I am in Love",
             track: "Hiding In Your Insides (php)",
             source: "Mokele.co.uk",
             url: "http://play.mokele.co.uk/music/Hiding%20In%20Your%20Insides.mp3",
             bitrate: 160,
             duration: 248,
             size: 4971780,
             score: 1.0,
             extension: "mp3",
             mimetype: "audio/mpeg"
 }
 ]
 };
 }
 }
 );

 // register the resolver
 Tomahawk.resolver.instance = DemoResolver;*/

/**** end example implementation of a resolver ****/


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
 * Do a synchronous HTTP(S) request. For further options see
 * Tomahawk.asyncRequest
 */
Tomahawk.syncRequest = function (url, extraHeaders, options) {
    // unpack options
    var opt = options || {};
    var method = opt.method || 'GET';

    var xmlHttpRequest = new XMLHttpRequest();
    xmlHttpRequest.open(method, url, false, opt.username, opt.password);
    if (extraHeaders) {
        for (var headerName in extraHeaders) {
            xmlHttpRequest.setRequestHeader(headerName, extraHeaders[headerName]);
        }
    }
    xmlHttpRequest.send(null);
    if (xmlHttpRequest.status == 200) {
        return xmlHttpRequest.responseText;
    } else {
        Tomahawk.log("Failed to do GET request: to: " + url);
        Tomahawk.log("Status Code was: " + xmlHttpRequest.status);
        if (opt.hasOwnProperty('errorHandler')) {
            opt.errorHandler.call(window, xmlHttpRequest);
        }
    }
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
Tomahawk.retrievedMetadata = function(metadataId, metadata, error) {
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
 * Pass the natively retrived reply back to the javascript callback
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
    if (xhr.readyState == 4 && xhr.status == 200) {
        // Call the real callback
        if (Tomahawk.asyncRequestCallbacks[reqId].callback) {
            Tomahawk.asyncRequestCallbacks[reqId].callback(xhr);
        }
    } else if (xmlHttpRequest.readyState === 4) {
        Tomahawk.log("Failed to do nativeAsyncRequest");
        Tomahawk.log("Status Code was: " + xhr.status);
        if (Tomahawk.asyncRequestCallbacks[reqId].errorHandler) {
            Tomahawk.asyncRequestCallbacks[reqId].errorHandler(xhr);
        }
    }

    // Callbacks are only used once.
    delete Tomahawk.asyncRequestCallbacks[reqId];
};

/**
 * Possible options:
 *  - method: The HTTP request method (default: GET)
 *  - username: The username for HTTP Basic Auth
 *  - password: The password for HTTP Basic Auth
 *  - errorHandler: callback called if the request was not completed
 *  - data: body data included in POST requests
 *  - needCookieHeader: boolean indicating whether or not the request needs to be able to get the
 *                      "Set-Cookie" response header
 */
Tomahawk.asyncRequest = function (url, callback, extraHeaders, options) {
    // unpack options
    var opt = options || {};
    var method = opt.method || 'GET';

    if (shouldDoNativeRequest(url, callback, extraHeaders, options)) {
        // Assign a request Id to the callback so we can use it when we are
        // returning from the native call.
        var reqId = Tomahawk.asyncRequestIdCounter;
        Tomahawk.asyncRequestIdCounter++;
        Tomahawk.asyncRequestCallbacks[reqId] = {
            callback: callback,
            errorHandler: opt.errorHandler
        };
        Tomahawk.nativeAsyncRequest(reqId, url, extraHeaders, options);
    } else {
        var xmlHttpRequest = new XMLHttpRequest();
        xmlHttpRequest.open(method, url, true, opt.username, opt.password);
        if (extraHeaders) {
            for (var headerName in extraHeaders) {
                xmlHttpRequest.setRequestHeader(headerName, extraHeaders[headerName]);
            }
        }
        xmlHttpRequest.onreadystatechange = function () {
            if (xmlHttpRequest.readyState == 4 && xmlHttpRequest.status == 200) {
                callback.call(window, xmlHttpRequest);
            } else if (xmlHttpRequest.readyState === 4) {
                Tomahawk.log("Failed to do " + method + " request: to: " + url);
                Tomahawk.log("Status Code was: " + xmlHttpRequest.status);
                if (opt.hasOwnProperty('errorHandler')) {
                    opt.errorHandler.call(window, xmlHttpRequest);
                }
            }
        };
        xmlHttpRequest.send(opt.data || null);
    }
};

/**
 * This method is externalized from Tomahawk.asyncRequest, so that other clients
 * (like tomahawk-android) can inject their own logic that determines whether or not to do a request
 * natively.
 *
 * @returns boolean indicating whether or not to do a request with the given parameters natively
 */
var shouldDoNativeRequest = function (url, callback, extraHeaders, options) {
    return (extraHeaders && (extraHeaders.hasOwnProperty("Referer")
        || extraHeaders.hasOwnProperty("referer")));
};

Tomahawk.ajax = function(url, settings) {
    if (typeof url === "object") {
        settings = url;
    } else {
        settings = settings || {};
        settings.url = url;
    }

    settings.type = settings.type || settings.method || 'get';
    settings.method = settings.type;
    settings.dataFormat = settings.dataFormat || 'form';

    if (settings.data) {
        var formEncode = function(obj) {
            var str = [];
            for(var p in obj) {
                if(obj[p] !== undefined) {
                    str.push(encodeURIComponent(p) + "=" + encodeURIComponent(obj[p]));
                }
            }

            str.sort();

            return str.join("&");
        };
        if (typeof settings.data === 'object') {
            if (settings.dataFormat == 'form') {
                settings.data = formEncode(settings.data);
                settings.contentType = settings.contentType || 'application/x-www-form-urlencoded';
            } else if (settings.dataFormat == 'json') {
                settings.data = JSON.stringify(settings.data);
                settings.contentType = settings.contentType || 'application/json';
            } else {
                throw new Error("Tomahawk.ajax: unknown dataFormat requested: " + settings.dataFormat);
            }
        } else {
            throw new Error("Tomahawk.ajax: data should be either object or string");
        }

        if (settings.type.toLowerCase() === 'get') {
            settings.url += '?' + settings.data;
            delete settings.data;
        } else {
            settings.headers = settings.headers || {};
            if (!settings.headers.hasOwnProperty('Content-Type')) {
                settings.headers['Content-Type'] = settings.contentType;
            }
        }
    }

    return new Promise(function (resolve, reject) {
        settings.errorHandler = reject;
        Tomahawk.asyncRequest(settings.url, resolve, settings.headers, settings);
    }).then(function(xhr) {
        var responseText = xhr.responseText;
        var contentType;
        if (settings.dataType === 'json') {
            contentType = 'application/json';
        } else if (contentType === 'xml') {
            contentType = 'text/xml';
        } else {
            contentType = xhr.getResponseHeader('Content-Type');
        }

        if (~contentType.indexOf('application/json')) {
            return JSON.parse(responseText);
        }

        if (~contentType.indexOf('text/xml')) {
            var domParser = new DOMParser();
            return domParser.parseFromString(responseText, "text/xml");
        }

        return xhr.responseText;
    });
};

Tomahawk.post = function(url, settings) {
    if (typeof url === "object") {
        settings = url;
    } else {
        settings = settings || {};
        settings.url = url;
    }

    settings.method = 'POST';

    return Tomahawk.ajax(settings);
};

Tomahawk.get = function(url, settings) {
    return Tomahawk.ajax(url, settings);
};

Tomahawk.assert = function (assertion, message) {
    Tomahawk.nativeAssert(assertion, message);
};

Tomahawk.sha256 = Tomahawk.sha256 || function(message) {
  return CryptoJS.SHA256(message).toString(CryptoJS.enc.Hex);
};
Tomahawk.md5 = Tomahawk.md5 || function(message) {
  return CryptoJS.MD5(message).toString(CryptoJS.enc.Hex);
};
// Return a HMAC (md5) signature of the input text with the desired key
Tomahawk.hmac = function (key, message) {
    return CryptoJS.HmacMD5(message, key).toString(CryptoJS.enc.Hex);
};

// Extracted from https://github.com/andrewrk/diacritics version 1.2.0
// Thanks to Andrew Kelley for this MIT-licensed diacritic removal code
// Initialisation / precomputation
(function() {
    var replacementList = [
        {base: ' ', chars: "\u00A0"},
        {base: '0', chars: "\u07C0"},
        {base: 'A', chars: "\u24B6\uFF21\u00C0\u00C1\u00C2\u1EA6\u1EA4\u1EAA\u1EA8\u00C3\u0100\u0102\u1EB0\u1EAE\u1EB4\u1EB2\u0226\u01E0\u00C4\u01DE\u1EA2\u00C5\u01FA\u01CD\u0200\u0202\u1EA0\u1EAC\u1EB6\u1E00\u0104\u023A\u2C6F"},
        {base: 'AA', chars: "\uA732"},
        {base: 'AE', chars: "\u00C6\u01FC\u01E2"},
        {base: 'AO', chars: "\uA734"},
        {base: 'AU', chars: "\uA736"},
        {base: 'AV', chars: "\uA738\uA73A"},
        {base: 'AY', chars: "\uA73C"},
        {base: 'B', chars: "\u24B7\uFF22\u1E02\u1E04\u1E06\u0243\u0181"},
        {base: 'C', chars: "\uFF43\u24b8\uff23\uA73E\u1E08"},
        {base: 'D', chars: "\u24B9\uFF24\u1E0A\u010E\u1E0C\u1E10\u1E12\u1E0E\u0110\u018A\u0189\u1D05\uA779"},
        {base: 'Dh', chars: "\u00D0"},
        {base: 'DZ', chars: "\u01F1\u01C4"},
        {base: 'Dz', chars: "\u01F2\u01C5"},
        {base: 'E', chars: "\u025B\u24BA\uFF25\u00C8\u00C9\u00CA\u1EC0\u1EBE\u1EC4\u1EC2\u1EBC\u0112\u1E14\u1E16\u0114\u0116\u00CB\u1EBA\u011A\u0204\u0206\u1EB8\u1EC6\u0228\u1E1C\u0118\u1E18\u1E1A\u0190\u018E\u1D07"},
        {base: 'F', chars: "\uA77C\u24BB\uFF26\u1E1E\u0191\uA77B"},
        {base: 'G', chars: "\u24BC\uFF27\u01F4\u011C\u1E20\u011E\u0120\u01E6\u0122\u01E4\u0193\uA7A0\uA77D\uA77E\u0262"},
        {base: 'H', chars: "\u24BD\uFF28\u0124\u1E22\u1E26\u021E\u1E24\u1E28\u1E2A\u0126\u2C67\u2C75\uA78D"},
        {base: 'I', chars: "\u24BE\uFF29\xCC\xCD\xCE\u0128\u012A\u012C\u0130\xCF\u1E2E\u1EC8\u01CF\u0208\u020A\u1ECA\u012E\u1E2C\u0197"},
        {base: 'J', chars: "\u24BF\uFF2A\u0134\u0248\u0237"},
        {base: 'K', chars: "\u24C0\uFF2B\u1E30\u01E8\u1E32\u0136\u1E34\u0198\u2C69\uA740\uA742\uA744\uA7A2"},
        {base: 'L', chars: "\u24C1\uFF2C\u013F\u0139\u013D\u1E36\u1E38\u013B\u1E3C\u1E3A\u0141\u023D\u2C62\u2C60\uA748\uA746\uA780"},
        {base: 'LJ', chars: "\u01C7"},
        {base: 'Lj', chars: "\u01C8"},
        {base: 'M', chars: "\u24C2\uFF2D\u1E3E\u1E40\u1E42\u2C6E\u019C\u03FB"},
        {base: 'N', chars: "\uA7A4\u0220\u24C3\uFF2E\u01F8\u0143\xD1\u1E44\u0147\u1E46\u0145\u1E4A\u1E48\u019D\uA790\u1D0E"},
        {base: 'NJ', chars: "\u01CA"},
        {base: 'Nj', chars: "\u01CB"},
        {base: 'O', chars: "\u24C4\uFF2F\xD2\xD3\xD4\u1ED2\u1ED0\u1ED6\u1ED4\xD5\u1E4C\u022C\u1E4E\u014C\u1E50\u1E52\u014E\u022E\u0230\xD6\u022A\u1ECE\u0150\u01D1\u020C\u020E\u01A0\u1EDC\u1EDA\u1EE0\u1EDE\u1EE2\u1ECC\u1ED8\u01EA\u01EC\xD8\u01FE\u0186\u019F\uA74A\uA74C"},
        {base: 'OE', chars: "\u0152"},
        {base: 'OI', chars: "\u01A2"},
        {base: 'OO', chars: "\uA74E"},
        {base: 'OU', chars: "\u0222"},
        {base: 'P', chars: "\u24C5\uFF30\u1E54\u1E56\u01A4\u2C63\uA750\uA752\uA754"},
        {base: 'Q', chars: "\u24C6\uFF31\uA756\uA758\u024A"},
        {base: 'R', chars: "\u24C7\uFF32\u0154\u1E58\u0158\u0210\u0212\u1E5A\u1E5C\u0156\u1E5E\u024C\u2C64\uA75A\uA7A6\uA782"},
        {base: 'S', chars: "\u24C8\uFF33\u1E9E\u015A\u1E64\u015C\u1E60\u0160\u1E66\u1E62\u1E68\u0218\u015E\u2C7E\uA7A8\uA784"},
        {base: 'T', chars: "\u24C9\uFF34\u1E6A\u0164\u1E6C\u021A\u0162\u1E70\u1E6E\u0166\u01AC\u01AE\u023E\uA786"},
        {base: 'Th', chars: "\u00DE"},
        {base: 'TZ', chars: "\uA728"},
        {base: 'U', chars: "\u24CA\uFF35\xD9\xDA\xDB\u0168\u1E78\u016A\u1E7A\u016C\xDC\u01DB\u01D7\u01D5\u01D9\u1EE6\u016E\u0170\u01D3\u0214\u0216\u01AF\u1EEA\u1EE8\u1EEE\u1EEC\u1EF0\u1EE4\u1E72\u0172\u1E76\u1E74\u0244"},
        {base: 'V', chars: "\u24CB\uFF36\u1E7C\u1E7E\u01B2\uA75E\u0245"},
        {base: 'VY', chars: "\uA760"},
        {base: 'W', chars: "\u24CC\uFF37\u1E80\u1E82\u0174\u1E86\u1E84\u1E88\u2C72"},
        {base: 'X', chars: "\u24CD\uFF38\u1E8A\u1E8C"},
        {base: 'Y', chars: "\u24CE\uFF39\u1EF2\xDD\u0176\u1EF8\u0232\u1E8E\u0178\u1EF6\u1EF4\u01B3\u024E\u1EFE"},
        {base: 'Z', chars: "\u24CF\uFF3A\u0179\u1E90\u017B\u017D\u1E92\u1E94\u01B5\u0224\u2C7F\u2C6B\uA762"},
        {base: 'a', chars: "\u24D0\uFF41\u1E9A\u00E0\u00E1\u00E2\u1EA7\u1EA5\u1EAB\u1EA9\u00E3\u0101\u0103\u1EB1\u1EAF\u1EB5\u1EB3\u0227\u01E1\u00E4\u01DF\u1EA3\u00E5\u01FB\u01CE\u0201\u0203\u1EA1\u1EAD\u1EB7\u1E01\u0105\u2C65\u0250\u0251"},
        {base: 'aa', chars: "\uA733"},
        {base: 'ae', chars: "\u00E6\u01FD\u01E3"},
        {base: 'ao', chars: "\uA735"},
        {base: 'au', chars: "\uA737"},
        {base: 'av', chars: "\uA739\uA73B"},
        {base: 'ay', chars: "\uA73D"},
        {base: 'b', chars: "\u24D1\uFF42\u1E03\u1E05\u1E07\u0180\u0183\u0253\u0182"},
        {base: 'c', chars: "\u24D2\u0107\u0109\u010B\u010D\u00E7\u1E09\u0188\u023C\uA73F\u2184\u0043\u0106\u0108\u010A\u010C\u00C7\u0187\u023B"},
        {base: 'd', chars: "\u24D3\uFF44\u1E0B\u010F\u1E0D\u1E11\u1E13\u1E0F\u0111\u018C\u0256\u0257\u018B\u13E7\u0501\uA7AA"},
        {base: 'dh', chars: "\u00F0"},
        {base: 'dz', chars: "\u01F3\u01C6"},
        {base: 'e', chars: "\u24D4\uFF45\u00E8\u00E9\u00EA\u1EC1\u1EBF\u1EC5\u1EC3\u1EBD\u0113\u1E15\u1E17\u0115\u0117\u00EB\u1EBB\u011B\u0205\u0207\u1EB9\u1EC7\u0229\u1E1D\u0119\u1E19\u1E1B\u0247\u01DD"},
        {base: 'f', chars: "\u24D5\uFF46\u1E1F\u0192"},
        {base: 'ff', chars: "\uFB00"},
        {base: 'fi', chars: "\uFB01"},
        {base: 'fl', chars: "\uFB02"},
        {base: 'ffi', chars: "\uFB03"},
        {base: 'ffl', chars: "\uFB04"},
        {base: 'g', chars: "\u24D6\uFF47\u01F5\u011D\u1E21\u011F\u0121\u01E7\u0123\u01E5\u0260\uA7A1\uA77F\u1D79"},
        {base: 'h', chars: "\u24D7\uFF48\u0125\u1E23\u1E27\u021F\u1E25\u1E29\u1E2B\u1E96\u0127\u2C68\u2C76\u0265"},
        {base: 'hv', chars: "\u0195"},
        {base: 'i', chars: "\u24D8\uFF49\xEC\xED\xEE\u0129\u012B\u012D\xEF\u1E2F\u1EC9\u01D0\u0209\u020B\u1ECB\u012F\u1E2D\u0268\u0131"},
        {base: 'j', chars: "\u24D9\uFF4A\u0135\u01F0\u0249"},
        {base: 'k', chars: "\u24DA\uFF4B\u1E31\u01E9\u1E33\u0137\u1E35\u0199\u2C6A\uA741\uA743\uA745\uA7A3"},
        {base: 'l', chars: "\u24DB\uFF4C\u0140\u013A\u013E\u1E37\u1E39\u013C\u1E3D\u1E3B\u017F\u0142\u019A\u026B\u2C61\uA749\uA781\uA747\u026D"},
        {base: 'lj', chars: "\u01C9"},
        {base: 'm', chars: "\u24DC\uFF4D\u1E3F\u1E41\u1E43\u0271\u026F"},
        {base: 'n', chars: "\u24DD\uFF4E\u01F9\u0144\xF1\u1E45\u0148\u1E47\u0146\u1E4B\u1E49\u019E\u0272\u0149\uA791\uA7A5\u043B\u0509"},
        {base: 'nj', chars: "\u01CC"},
        {base: 'o', chars: "\u24DE\uFF4F\xF2\xF3\xF4\u1ED3\u1ED1\u1ED7\u1ED5\xF5\u1E4D\u022D\u1E4F\u014D\u1E51\u1E53\u014F\u022F\u0231\xF6\u022B\u1ECF\u0151\u01D2\u020D\u020F\u01A1\u1EDD\u1EDB\u1EE1\u1EDF\u1EE3\u1ECD\u1ED9\u01EB\u01ED\xF8\u01FF\uA74B\uA74D\u0275\u0254\u1D11"},
        {base: 'oe', chars: "\u0153"},
        {base: 'oi', chars: "\u01A3"},
        {base: 'oo', chars: "\uA74F"},
        {base: 'ou', chars: "\u0223"},
        {base: 'p', chars: "\u24DF\uFF50\u1E55\u1E57\u01A5\u1D7D\uA751\uA753\uA755\u03C1"},
        {base: 'q', chars: "\u24E0\uFF51\u024B\uA757\uA759"},
        {base: 'r', chars: "\u24E1\uFF52\u0155\u1E59\u0159\u0211\u0213\u1E5B\u1E5D\u0157\u1E5F\u024D\u027D\uA75B\uA7A7\uA783"},
        {base: 's', chars: "\u24E2\uFF53\u015B\u1E65\u015D\u1E61\u0161\u1E67\u1E63\u1E69\u0219\u015F\u023F\uA7A9\uA785\u1E9B\u0282"},
        {base: 'ss', chars: "\xDF"},
        {base: 't', chars: "\u24E3\uFF54\u1E6B\u1E97\u0165\u1E6D\u021B\u0163\u1E71\u1E6F\u0167\u01AD\u0288\u2C66\uA787"},
        {base: 'th', chars: "\u00FE"},
        {base: 'tz', chars: "\uA729"},
        {base: 'u', chars: "\u24E4\uFF55\xF9\xFA\xFB\u0169\u1E79\u016B\u1E7B\u016D\xFC\u01DC\u01D8\u01D6\u01DA\u1EE7\u016F\u0171\u01D4\u0215\u0217\u01B0\u1EEB\u1EE9\u1EEF\u1EED\u1EF1\u1EE5\u1E73\u0173\u1E77\u1E75\u0289"},
        {base: 'v', chars: "\u24E5\uFF56\u1E7D\u1E7F\u028B\uA75F\u028C"},
        {base: 'vy', chars: "\uA761"},
        {base: 'w', chars: "\u24E6\uFF57\u1E81\u1E83\u0175\u1E87\u1E85\u1E98\u1E89\u2C73"},
        {base: 'x', chars: "\u24E7\uFF58\u1E8B\u1E8D"},
        {base: 'y', chars: "\u24E8\uFF59\u1EF3\xFD\u0177\u1EF9\u0233\u1E8F\xFF\u1EF7\u1E99\u1EF5\u01B4\u024F\u1EFF"},
        {base: 'z', chars: "\u24E9\uFF5A\u017A\u1E91\u017C\u017E\u1E93\u1E95\u01B6\u0225\u0240\u2C6C\uA763"}
    ];

    Tomahawk.diacriticsMap = {};
    var i, j, chars;
    for (i = 0; i < replacementList.length; i += 1) {
        chars = replacementList[i].chars;
        for (j = 0; j < chars.length; j += 1) {
            Tomahawk.diacriticsMap[chars[j]] = replacementList[i].base;
        }
    }
})();

Tomahawk.removeDiacritics = function (str) {
    return str.replace(/[^\u0000-\u007E]/g, function(c) {
        return Tomahawk.diacriticsMap[c] || c;
    });
};

Tomahawk.localStorage = Tomahawk.localStorage || {
    setItem: function() {},
    getItem: function() {},
    removeItem: function() {}
};

// some aliases
Tomahawk.setTimeout = Tomahawk.setTimeout || window.setTimeout;
Tomahawk.setInterval = Tomahawk.setInterval || window.setInterval;
Tomahawk.base64Decode = function(a) { return window.atob(a); };
Tomahawk.base64Encode = function(b) { return window.btoa(b); };

Tomahawk.PluginManager = {
    objects: {},
    objectCounter: 0,
    identifyObject: function (object) {
        if( !object.hasOwnProperty('id') ) {
            object.id = this.objectCounter++;
        }

        return object.id;
    },
    registerPlugin: function (type, object) {
        this.objects[this.identifyObject(object)] = object;

        Tomahawk.log("registerPlugin: " + type + " id: " + object.id);
        Tomahawk.registerScriptPlugin(type, object.id);
    },

    unregisterPlugin: function(type, object) {
        this.objects[this.identifyObject(object)] = object;

        Tomahawk.log("unregisterPlugin: " + type + " id: " + object.id);
        Tomahawk.unregisterScriptPlugin(type, object.id);
    },

    resolve: [],
    invokeSync: function (requestId, objectId, methodName, params) {
        if (!Tomahawk.resolver.instance.apiVersion || Tomahawk.resolver.instance.apiVersion < 0.9) {
            if (methodName === 'artistAlbums') {
                methodName = 'albums';
            } else if ( methodName === 'albumTracks' ) {
                methodName = 'tracks';
            }
        }

        var pluginManager = this;
        if (!this.objects[objectId]) {
            Tomahawk.log("Object not found! objectId: " + objectId + " methodName: " + methodName);
        } else {
            if (!this.objects[objectId][methodName]) {
                Tomahawk.log("Function not found: " + methodName);
            }
        }

        if (typeof this.objects[objectId][methodName] === 'function') {
            if (!Tomahawk.resolver.instance.apiVersion || Tomahawk.resolver.instance.apiVersion < 0.9) {
                if (methodName == 'artists') {
                    return new Promise(function (resolve, reject) {
                        pluginManager.resolve[requestId] = resolve;
                        Tomahawk.resolver.instance.artists(requestId);
                    });
                } else if (methodName == 'albums') {
                    return new Promise(function (resolve, reject) {
                        pluginManager.resolve[requestId] = resolve;
                        Tomahawk.resolver.instance.albums(requestId, params.artist);
                    });
                } else if (methodName == 'tracks') {
                    return new Promise(function (resolve, reject) {
                        pluginManager.resolve[requestId] = resolve;
                        Tomahawk.resolver.instance.tracks(requestId, params.artist, params.album);
                    });
                }
            }

            return this.objects[objectId][methodName](params);
        }

        return this.objects[objectId][methodName];
    },

    invoke: function (requestId, objectId, methodName, params ) {
        Promise.resolve(this.invokeSync(requestId, objectId, methodName, params)).then(function (result) {
            if (typeof result === 'object') {
                Tomahawk.reportScriptJobResults({
                    requestId: requestId,
                    data: result
                });
            } else {
                Tomahawk.reportScriptJobResults({
                    requestId: requestId,
                    error: "Scripts need to return objects for requests: methodName: " + methodName + " params: " + JSON.stringify(params)
                });
            }
        }, function (error) {
            Tomahawk.reportScriptJobResults({
                requestId: requestId,
                error: error
            });
        });
    }
};


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
    }
};


// Legacy compability for 0.8 and before
Tomahawk.reportCapabilities = function (capabilities) {
    if (capabilities & TomahawkResolverCapability.Browsable) {
        Tomahawk.PluginManager.registerPlugin("collection", Tomahawk.resolver.instance);
    }

    Tomahawk.nativeReportCapabilities(capabilities);
};

Tomahawk.addArtistResults = Tomahawk.addAlbumResults = Tomahawk.addAlbumTrackResults = function (result) {
    Tomahawk.PluginManager.resolve[result.qid](result);
    delete Tomahawk.PluginManager.resolve[result.qid];
};
