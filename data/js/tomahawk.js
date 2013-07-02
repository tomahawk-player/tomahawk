/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011,      Dominik Schmidt <domme@tomahawk-player.org>
 *   Copyright 2011-2012, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

// if run in phantomjs add fake Tomahawk environment
if(window.Tomahawk === undefined)
{
//    alert("PHANTOMJS ENVIRONMENT");
    var Tomahawk = {
        fakeEnv: function()
        {
            return true;
        },
        resolverData: function()
        {
            return {
                scriptPath: function()
                {
                    return "/home/tomahawk/resolver.js";
                }
            };
        },
        log: function( message )
        {
            console.log( message );
        }
    };
}


Tomahawk.resolver = {
    scriptPath: Tomahawk.resolverData().scriptPath
};

Tomahawk.timestamp = function() {
    return Math.round( new Date()/1000 );
};

Tomahawk.dumpResult = function( result ) {
    var results = result.results;
    Tomahawk.log("Dumping " + results.length + " results for query " + result.qid + "...");
    for(var i=0; i<results.length;i++)
    {
        var result1 = results[i];
        Tomahawk.log( result1.artist + " - " + result1.track + " | " + result1.url );
    }

    Tomahawk.log("Done.");
};

// javascript part of Tomahawk-Object API
Tomahawk.extend = function(object, members) {
    var F = function() {};
    F.prototype = object;
    var newObject = new F;

    for(var key in members)
    {
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
    init: function()
    {
    },
    scriptPath: function()
    {
        return Tomahawk.resolverData().scriptPath;
    },
    getConfigUi: function()
    {
        return {};
    },
    getUserConfig: function()
    {
        var configJson = window.localStorage[ this.scriptPath() ];
        if( configJson === undefined )
	{
            configJson = "{}";
	}

        var config = JSON.parse( configJson );

        return config;
    },
    saveUserConfig: function()
    {
        var config = Tomahawk.resolverData().config;
        var configJson = JSON.stringify( config );

        window.localStorage[ this.scriptPath() ] = configJson;

        this.newConfigSaved();
    },
    newConfigSaved: function()
    {
    },
    resolve: function( qid, artist, album, title )
    {
        return {
            qid: qid
        };
    },
    search: function( qid, searchString )
    {
        return this.resolve( qid, "", "", searchString );
    },
    artists: function( qid )
    {
        return {
            qid: qid
        };
    },
    albums: function( qid, artist )
    {
        return {
            qid: qid
        };
    },
    tracks: function( qid, artist, album )
    {
        return {
            qid: qid
        };
    },
    collection: function()
    {
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

Tomahawk.valueForSubNode = function(node, tag)
{
    if(node === undefined)
    {
        throw new Error("Tomahawk.valueForSubnode: node is undefined!");
    }

    var element = node.getElementsByTagName(tag)[0];
    if( element === undefined )
    {
        return undefined;
    }

    return element.textContent;
};


Tomahawk.syncRequest = function(url)
{
	var xmlHttpRequest = new XMLHttpRequest();
	xmlHttpRequest.open('GET', url, false);
	xmlHttpRequest.send(null);
	if (xmlHttpRequest.status == 200){
		return xmlHttpRequest.responseText;
	}
};

Tomahawk.asyncRequest = function(url, callback, extraHeaders)
{
    var xmlHttpRequest = new XMLHttpRequest();
    xmlHttpRequest.open('GET', url, true);
    if (extraHeaders) {
        for(var headerName in extraHeaders) {
            xmlHttpRequest.setRequestHeader(headerName, extraHeaders[headerName]);
        }
    }
    xmlHttpRequest.onreadystatechange = function() {
        if (xmlHttpRequest.readyState == 4 && xmlHttpRequest.status == 200) {
            callback.call(window, xmlHttpRequest);
        } else if (xmlHttpRequest.readyState === 4) {
            Tomahawk.log("Failed to do GET request: to: " + url);
            Tomahawk.log("Status Code was: " + xmlHttpRequest.status);
        }
    }
    xmlHttpRequest.send(null);
};

Tomahawk.sha256 = CryptoJS.SHA256;

// some aliases
Tomahawk.setTimeout = window.setTimeout;
Tomahawk.setInterval = window.setInterval;
