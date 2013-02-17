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
    AccountFactory: 4
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

/**
*
* Secure Hash Algorithm (SHA256)
* http://www.webtoolkit.info/
*
* Original code by Angel Marin, Paul Johnston.
*
**/

Tomahawk.sha256=function(s){

    var chrsz = 8;
    var hexcase = 0;

    function safe_add (x, y) {
        var lsw = (x & 0xFFFF) + (y & 0xFFFF);
        var msw = (x >> 16) + (y >> 16) + (lsw >> 16);
        return (msw << 16) | (lsw & 0xFFFF);
    }

    function S (X, n) { return ( X >>> n ) | (X << (32 - n)); }
    function R (X, n) { return ( X >>> n ); }
    function Ch(x, y, z) { return ((x & y) ^ ((~x) & z)); }
    function Maj(x, y, z) { return ((x & y) ^ (x & z) ^ (y & z)); }
    function Sigma0256(x) { return (S(x, 2) ^ S(x, 13) ^ S(x, 22)); }
    function Sigma1256(x) { return (S(x, 6) ^ S(x, 11) ^ S(x, 25)); }
    function Gamma0256(x) { return (S(x, 7) ^ S(x, 18) ^ R(x, 3)); }
    function Gamma1256(x) { return (S(x, 17) ^ S(x, 19) ^ R(x, 10)); }

    function core_sha256 (m, l) {
        var K = new Array(0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5, 0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174, 0xE49B69C1, 0xEFBE4786, 0xFC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA, 0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x6CA6351, 0x14292967, 0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85, 0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070, 0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3, 0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2);
        var HASH = new Array(0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19);
        var W = new Array(64);
        var a, b, c, d, e, f, g, h, i, j;
        var T1, T2;

        m[l >> 5] |= 0x80 << (24 - l % 32);
        m[((l + 64 >> 9) << 4) + 15] = l;

        for ( i = 0; i<m.length; i+=16 ) {
            a = HASH[0];
            b = HASH[1];
            c = HASH[2];
            d = HASH[3];
            e = HASH[4];
            f = HASH[5];
            g = HASH[6];
            h = HASH[7];

            for ( j = 0; j<64; j++) {
                if (j < 16)
		{
			W[j] = m[j + i];
		}
                else
		{
			W[j] = safe_add(safe_add(safe_add(Gamma1256(W[j - 2]), W[j - 7]), Gamma0256(W[j - 15])), W[j - 16]);
		}

                T1 = safe_add(safe_add(safe_add(safe_add(h, Sigma1256(e)), Ch(e, f, g)), K[j]), W[j]);
                T2 = safe_add(Sigma0256(a), Maj(a, b, c));

                h = g;
                g = f;
                f = e;
                e = safe_add(d, T1);
                d = c;
                c = b;
                b = a;
                a = safe_add(T1, T2);
            }

            HASH[0] = safe_add(a, HASH[0]);
            HASH[1] = safe_add(b, HASH[1]);
            HASH[2] = safe_add(c, HASH[2]);
            HASH[3] = safe_add(d, HASH[3]);
            HASH[4] = safe_add(e, HASH[4]);
            HASH[5] = safe_add(f, HASH[5]);
            HASH[6] = safe_add(g, HASH[6]);
            HASH[7] = safe_add(h, HASH[7]);
        }
        return HASH;
    }

    function str2binb (str) {
        var bin = Array();
        var mask = (1 << chrsz) - 1;
        for(var i = 0; i < str.length * chrsz; i += chrsz) {
            bin[i>>5] |= (str.charCodeAt(i / chrsz) & mask) << (24 - i%32);
        }
        return bin;
    }

    function Utf8Encode(string) {
        string = string.replace(/\r\n/g,"\n");
        var utftext = "";

        for (var n = 0; n < string.length; n++) {

            var c = string.charCodeAt(n);

            if (c < 128) {
                utftext += String.fromCharCode(c);
            }
            else if((c > 127) && (c < 2048)) {
                utftext += String.fromCharCode((c >> 6) | 192);
                utftext += String.fromCharCode((c & 63) | 128);
            }
            else {
                utftext += String.fromCharCode((c >> 12) | 224);
                utftext += String.fromCharCode(((c >> 6) & 63) | 128);
                utftext += String.fromCharCode((c & 63) | 128);
            }

        }

        return utftext;
    }

    function binb2hex (binarray) {
        var hex_tab = hexcase ? "0123456789ABCDEF" : "0123456789abcdef";
        var str = "";
        for(var i = 0; i < binarray.length * 4; i++) {
            str += hex_tab.charAt((binarray[i>>2] >> ((3 - i%4)*8+4)) & 0xF) +
            hex_tab.charAt((binarray[i>>2] >> ((3 - i%4)*8 )) & 0xF);
        }
        return str;
    }

    s = Utf8Encode(s);
    return binb2hex(core_sha256(str2binb(s), s.length * chrsz));

};



// some aliases
Tomahawk.setTimeout = window.setTimeout;
Tomahawk.setInterval = window.setInterval;
