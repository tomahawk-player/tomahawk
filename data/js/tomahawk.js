/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011,      Dominik Schmidt <domme@tomahawk-player.org>
 *   Copyright 2011-2012, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2011,      Thierry Goeckel
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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
        log: function (message) {
            console.log(message);
        }
    };
}

Tomahawk.apiVersion = "0.2.1";

/**
 * Compares versions strings
 * (version1 < version2) == -1
 * (version1 = version2) == 0
 * (version1 > version2) == 1
 */
Tomahawk.versionCompare = function (version1, version2) {
    var v1 = version1.split('.').map(function (item) { return parseInt(item); });
    var v2 = version2.split('.').map(function (item) { return parseInt(item); })
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
        } else if (v2[i] < v2[i]) {
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
    return (Tomahawk.versionCompare(version, Tomahawk.apiVersion) >= 0)
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
    }
};

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
 * Possible options:
 *  - method: The HTTP request method (default: GET)
 *  - username: The username for HTTP Basic Auth
 *  - password: The password for HTTP Basic Auth
 *  - errorHandler: callback called if the request was not completed
 *  - data: body data included in POST requests
 */
Tomahawk.asyncRequest = function (url, callback, extraHeaders, options) {
    // unpack options
    var opt = options || {};
    var method = opt.method || 'GET';

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
            Tomahawk.log("Failed to do GET request: to: " + url);
            Tomahawk.log("Status Code was: " + xmlHttpRequest.status);
            if (opt.hasOwnProperty('errorHandler')) {
                opt.errorHandler.call(window, xmlHttpRequest);
            }
        }
    };
    xmlHttpRequest.send(opt.data || null);
};

Tomahawk.sha256 = Tomahawk.sha256 || CryptoJS.SHA256;

// some aliases
Tomahawk.setTimeout = Tomahawk.setTimeout || window.setTimeout;
Tomahawk.setInterval = Tomahawk.setInterval || window.setInterval;
