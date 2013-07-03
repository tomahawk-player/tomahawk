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
if (window.Tomahawk === undefined) {
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


// Resolver BaseObject, inherit it to implement your own resolver
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


Tomahawk.syncRequest = function (url) {
	var xmlHttpRequest = new XMLHttpRequest();
	xmlHttpRequest.open('GET', url, false);
	xmlHttpRequest.send(null);
    if (xmlHttpRequest.status == 200) {
		return xmlHttpRequest.responseText;
	}
};

Tomahawk.asyncRequest = function (url, callback, extraHeaders) {
    var xmlHttpRequest = new XMLHttpRequest();
    xmlHttpRequest.open('GET', url, true);
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
        }
    };
    xmlHttpRequest.send(null);
};

Tomahawk.sha256 = CryptoJS.SHA256;

// some aliases
Tomahawk.setTimeout = window.setTimeout;
Tomahawk.setInterval = window.setInterval;
