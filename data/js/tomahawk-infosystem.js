// install ES6Promise as global Promise
if(window.Promise === undefined) {
    window.Promise = window.ES6Promise.Promise;
}

// TODO: find a way to enumerate TypeInfo instead of copying this manually

Tomahawk.InfoSystem.InfoType = Object.create(null);

Tomahawk.InfoSystem.InfoType.InfoNoInfo = 0; //WARNING: *ALWAYS* keep this first!
Tomahawk.InfoSystem.InfoType.InfoTrackID = 1;
Tomahawk.InfoSystem.InfoType.InfoTrackArtist = 2;
Tomahawk.InfoSystem.InfoType.InfoTrackAlbum = 3;
Tomahawk.InfoSystem.InfoType.InfoTrackGenre = 4;
Tomahawk.InfoSystem.InfoType.InfoTrackComposer = 5;
Tomahawk.InfoSystem.InfoType.InfoTrackDate = 6;
Tomahawk.InfoSystem.InfoType.InfoTrackNumber = 7;
Tomahawk.InfoSystem.InfoType.InfoTrackDiscNumber = 8;
Tomahawk.InfoSystem.InfoType.InfoTrackBitRate = 9;
Tomahawk.InfoSystem.InfoType.InfoTrackLength = 10;
Tomahawk.InfoSystem.InfoType.InfoTrackSampleRate = 11;
Tomahawk.InfoSystem.InfoType.InfoTrackFileSize = 12;
Tomahawk.InfoSystem.InfoType.InfoTrackBPM = 13;
Tomahawk.InfoSystem.InfoType.InfoTrackReplayGain = 14;
Tomahawk.InfoSystem.InfoType.InfoTrackReplayPeakGain = 15;
Tomahawk.InfoSystem.InfoType.InfoTrackLyrics = 16;
Tomahawk.InfoSystem.InfoType.InfoTrackLocation = 17;
Tomahawk.InfoSystem.InfoType.InfoTrackProfile = 18;
Tomahawk.InfoSystem.InfoType.InfoTrackEnergy = 19;
Tomahawk.InfoSystem.InfoType.InfoTrackDanceability = 20;
Tomahawk.InfoSystem.InfoType.InfoTrackTempo = 21;
Tomahawk.InfoSystem.InfoType.InfoTrackLoudness = 22;
Tomahawk.InfoSystem.InfoType.InfoTrackSimilars = 23; // cached -- do not change

Tomahawk.InfoSystem.InfoType.InfoArtistID = 25;
Tomahawk.InfoSystem.InfoType.InfoArtistName = 26;
Tomahawk.InfoSystem.InfoType.InfoArtistBiography = 27;
Tomahawk.InfoSystem.InfoType.InfoArtistImages = 28; //cached -- do not change
Tomahawk.InfoSystem.InfoType.InfoArtistBlog = 29;
Tomahawk.InfoSystem.InfoType.InfoArtistFamiliarity = 30;
Tomahawk.InfoSystem.InfoType.InfoArtistHotttness = 31;
Tomahawk.InfoSystem.InfoType.InfoArtistSongs = 32; //cached -- do not change
Tomahawk.InfoSystem.InfoType.InfoArtistSimilars = 33; //cached -- do not change
Tomahawk.InfoSystem.InfoType.InfoArtistNews = 34;
Tomahawk.InfoSystem.InfoType.InfoArtistProfile = 35;
Tomahawk.InfoSystem.InfoType.InfoArtistReviews = 36;
Tomahawk.InfoSystem.InfoType.InfoArtistTerms = 37;
Tomahawk.InfoSystem.InfoType.InfoArtistLinks = 38;
Tomahawk.InfoSystem.InfoType.InfoArtistVideos = 39;
Tomahawk.InfoSystem.InfoType.InfoArtistReleases = 40;

Tomahawk.InfoSystem.InfoType.InfoAlbumID = 42;
Tomahawk.InfoSystem.InfoType.InfoAlbumCoverArt = 43; //cached -- do not change
Tomahawk.InfoSystem.InfoType.InfoAlbumName = 44;
Tomahawk.InfoSystem.InfoType.InfoAlbumArtist = 45;
Tomahawk.InfoSystem.InfoType.InfoAlbumDate = 46;
Tomahawk.InfoSystem.InfoType.InfoAlbumGenre = 47;
Tomahawk.InfoSystem.InfoType.InfoAlbumComposer = 48;
Tomahawk.InfoSystem.InfoType.InfoAlbumSongs = 49;

Tomahawk.InfoSystem.InfoType.InfoChartCapabilities = 50;

Tomahawk.InfoSystem.InfoType.InfoChart = 51;

Tomahawk.InfoSystem.InfoType.InfoNewReleaseCapabilities = 52;
Tomahawk.InfoSystem.InfoType.InfoNewRelease = 53;

Tomahawk.InfoSystem.InfoType.InfoMiscTopHotttness = 60;
Tomahawk.InfoSystem.InfoType.InfoMiscTopTerms = 61;

Tomahawk.InfoSystem.InfoType.InfoSubmitNowPlaying = 70;
Tomahawk.InfoSystem.InfoType.InfoSubmitScrobble = 71;

Tomahawk.InfoSystem.InfoType.InfoNowPlaying = 80;
Tomahawk.InfoSystem.InfoType.InfoNowPaused = 81;
Tomahawk.InfoSystem.InfoType.InfoNowResumed = 82;
Tomahawk.InfoSystem.InfoType.InfoNowStopped = 83;
Tomahawk.InfoSystem.InfoType.InfoTrackUnresolved = 84;

Tomahawk.InfoSystem.InfoType.InfoLove = 90;
Tomahawk.InfoSystem.InfoType.InfoUnLove = 91;
Tomahawk.InfoSystem.InfoType.InfoShareTrack = 92;

Tomahawk.InfoSystem.InfoType.InfoNotifyUser = 100;

Tomahawk.InfoSystem.InfoType.InfoInboxReceived = 101;

Tomahawk.InfoSystem.InfoType.InfoLastInfo = 102; //WARNING: *ALWAYS* keep this last!

// PushInfoFlags
Tomahawk.InfoSystem.PushInfoFlags = Object.create(null);
Tomahawk.InfoSystem.PushInfoFlags.PushNoFlag = 1;
Tomahawk.InfoSystem.PushInfoFlags.PushShortUrlFlag = 2;


Tomahawk.InfoSystem._infoPluginIdCounter = 0;
Tomahawk.InfoSystem._infoPluginHash = Object.create(null);

Tomahawk.InfoSystem.addInfoPlugin = function(infoPlugin) {
    var infoPluginId = Tomahawk.InfoSystem._infoPluginIdCounter++;
    Tomahawk.InfoSystem._infoPluginHash[infoPluginId] = infoPlugin;
    Tomahawk.log("Call nativeAddInfoPlugin");
    Tomahawk.InfoSystem.nativeAddInfoPlugin(infoPluginId);
};

Tomahawk.InfoSystem.getInfoPlugin = function (infoPluginId) {
    return Tomahawk.InfoSystem._infoPluginHash[infoPluginId];
};

Tomahawk.InfoSystem.removeInfoPlugin = function (infoPluginId) {
    Tomahawk.log('Removing info plugins from JS is not implemented yet');
    Tomahawk.assert(false);
};

Tomahawk.InfoSystem.InfoPlugin = {
    infoTypeString: function(infoType) {
        for (var currentInfoTypeString in Tomahawk.InfoSystem.InfoType) {
            if (Tomahawk.InfoSystem.InfoType[currentInfoTypeString] === infoType) {
                return currentInfoTypeString;
            }
        }
    },
    // we can get around infoPluginId here probably ... but internal either way
    _notInCache: function (infoPluginId, requestId, requestType, criteria) {
        this.notInCache(requestType, criteria).then(function(result) {
            Tomahawk.log("Call nativeAddInfoRequestResult");
            Tomahawk.InfoSystem.nativeAddInfoRequestResult(infoPluginId, requestId, result.maxAge, result.data);
        }).catch(function() {
            // TODO: how to handle errors here?!
        });
    },
    notInCache: function (infoType, criteria) {
        var requestMethod = 'request' + this.infoTypeString(infoType);
        Tomahawk.log('Calling requestMethod: ' + requestMethod);

        return Promise.resolve(this[requestMethod](criteria));
    },
    pushInfo: function (pushData) {
        var pushMethod = 'push' + this.infoTypeString(pushData.type);
        return this[pushMethod](pushData);
    },
    // we can get around infoPluginId here probably ... but internal either way
    _getInfo: function (infoPluginId, requestId, type, infoHash) {
        Tomahawk.log("currentInfoPlugin._getInfo");
        window.getInfo = arguments;
        this.getInfo(type, infoHash).then(function(result) {
            Tomahawk.log("Call nativeGetCachedInfo");
            Tomahawk.InfoSystem.nativeGetCachedInfo(infoPluginId, requestId, result.newMaxAge, result.criteria)
        }, function() {
            Tomahawk.log("Call nativeDataError");
            Tomahawk.InfoSystem.nativeDataError();
        });
    },
    getInfo: function (type, infoHash) {
        Tomahawk.log("currentInfoPlugin.getInfo");
        var getInfoMethod = 'get' + this.infoTypeString(type);

        return Promise.resolve(this[getInfoMethod](infoHash));
    }
};
