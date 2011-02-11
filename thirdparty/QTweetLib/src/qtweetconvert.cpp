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

#include "qtweetconvert.h"
#include "qtweetstatus.h"
#include "qtweetdmstatus.h"
#include "qtweetuser.h"
#include "qtweetlist.h"
#include "qtweetplace.h"
#include "qtweetsearchresult.h"
#include "qtweetsearchpageresults.h"
#include "qtweetentityurl.h"
#include "qtweetentityhashtag.h"
#include "qtweetentityusermentions.h"

/**
 *  Converts list of statuses
 */
QList<QTweetStatus> QTweetConvert::variantToStatusList(const QVariant &fromParser)
{
    QList<QTweetStatus> statuses;

    QList<QVariant> listStatus = fromParser.toList();

    foreach (const QVariant& status, listStatus) {
        QVariantMap statusMap = status.toMap();

        QTweetStatus tweetStatus = variantMapToStatus(statusMap);

        statuses.append(tweetStatus);
    }
    return statuses;
}

/**
 *  Converts status
 */
QTweetStatus QTweetConvert::variantMapToStatus(const QVariantMap &var)
{
    QTweetStatus status;

    status.setCreatedAt(var["created_at"].toString());
    status.setText(var["text"].toString());
    status.setId(var["id"].toLongLong());
    status.setInReplyToUserId(var["in_reply_to_user_id"].toLongLong());
    status.setInReplyToScreenName(var["in_reply_to_screen_name"].toString());

    QVariantMap userMap = var["user"].toMap();
    QTweetUser user = variantMapToUserInfo(userMap);

    status.setUser(user);
    status.setSource(var["source"].toString());
    status.setInReplyToStatusId(var["in_reply_to_status_id"].toLongLong());

    //check if contains native retweet
    if (var.contains("retweeted_status")) {
        QVariantMap retweetMap = var["retweeted_status"].toMap();

        QTweetStatus rtStatus = variantMapToStatus(retweetMap);

        status.setRetweetedStatus(rtStatus);
    }

    //parse place id if it's not null
    QVariant placeVar = var["place"];
    if (!placeVar.isNull()) {
        QTweetPlace place = variantMapToPlace(placeVar.toMap());
        status.setPlace(place);
    }

    //check if contains entities
    if (var.contains("entities")) {
        QVariantMap entitiesVarMap = var["entities"].toMap();

        //url entities
        QVariantList urlEntitiesVarList = entitiesVarMap["urls"].toList();

        foreach (const QVariant& urlEntityVar, urlEntitiesVarList) {
            QVariantMap urlEntityVarMap = urlEntityVar.toMap();
            QTweetEntityUrl urlEntity = variantMapToEntityUrl(urlEntityVarMap);

            status.addUrlEntity(urlEntity);
        }

        //hashtag entities
        QVariantList hashtagEntitiesVarList = entitiesVarMap["hashtags"].toList();

        foreach (const QVariant& hashtagEntityVar, hashtagEntitiesVarList) {
            QVariantMap hashtagEntityVarMap = hashtagEntityVar.toMap();
            QTweetEntityHashtag hashtagEntity = variantMapToEntityHashtag(hashtagEntityVarMap);

            status.addHashtagEntity(hashtagEntity);
        }

        //user mentions
        QVariantList userMentionsEntitiesVarList = entitiesVarMap["user_mentions"].toList();

        foreach (const QVariant& userMentionsEntityVar, userMentionsEntitiesVarList) {
            QVariantMap userMentionsEntityVarMap = userMentionsEntityVar.toMap();
            QTweetEntityUserMentions userMentionsEntity = variantMapToEntityUserMentions(userMentionsEntityVarMap);

            status.addUserMentionsEntity(userMentionsEntity);
        }
    }

    return status;
}

/**
 *  Converts user info
 */
QTweetUser QTweetConvert::variantMapToUserInfo(const QVariantMap &var)
{
    QTweetUser userInfo;

    userInfo.setId(var["id"].toLongLong());

    //don't fill rest of it, when user info is trimmed
    if (var.contains("name")) {

        userInfo.setName(var["name"].toString());
        userInfo.setLocation(var["location"].toString());
        userInfo.setprofileImageUrl(var["profile_image_url"].toString());
        userInfo.setCreatedAt(var["created_at"].toString());
        userInfo.setFavouritesCount(var["favourites_count"].toInt());
        userInfo.setUrl(var["url"].toString());
        userInfo.setUtcOffset(var["utc_offset"].toInt());
        userInfo.setProtected(var["protected"].toBool());
        userInfo.setFollowersCount(var["followers_count"].toInt());
        userInfo.setVerified(var["verified"].toBool());
        userInfo.setGeoEnabled(var["geo_enabled"].toBool());
        userInfo.setDescription(var["description"].toString());
        userInfo.setTimezone(var["time_zone"].toString());
        userInfo.setFriendsCount(var["friends_count"].toInt());
        userInfo.setStatusesCount(var["statuses_count"].toInt());
        userInfo.setScreenName(var["screen_name"].toString());
        userInfo.setContributorsEnabled(var["contributors_enabled"].toBool());
        userInfo.setListedCount(var["listed_count"].toInt());
        userInfo.setLang(var["lang"].toString());

        //check if contains status
        if (var.contains("status")) {
            QVariantMap statusMap = var["status"].toMap();

            QTweetStatus status = variantMapToStatus(statusMap);

            userInfo.setStatus(status);
        }
    }

    return userInfo;
}

/**
 *  Converts list of direct messages
 */
QList<QTweetDMStatus> QTweetConvert::variantToDirectMessagesList(const QVariant& fromParser)
{
    QList<QTweetDMStatus> directMessages;

    QList<QVariant> listMessages = fromParser.toList();

    foreach (const QVariant& message, listMessages) {
        QTweetDMStatus dmStatus = variantMapToDirectMessage(message.toMap());
        directMessages.append(dmStatus);
    }

    return directMessages;
}

/**
 *  Converts direct message
 */
QTweetDMStatus QTweetConvert::variantMapToDirectMessage(const QVariantMap& var)
{
    QTweetDMStatus directMessage;

    directMessage.setCreatedAt(var["created_at"].toString());
    directMessage.setSenderScreenName(var["sender_screen_name"].toString());

    QVariantMap senderMap = var["sender"].toMap();
    QTweetUser sender = variantMapToUserInfo(senderMap);

    directMessage.setSender(sender);

    directMessage.setText(var["text"].toString());
    directMessage.setRecipientScreenName(var["recipient_screen_name"].toString());
    directMessage.setId(var["id"].toLongLong());

    QVariantMap recipientMap = var["recipient"].toMap();
    QTweetUser recipient = variantMapToUserInfo(recipientMap);

    directMessage.setRecipient(recipient);

    directMessage.setRecipientId(var["recipient_id"].toLongLong());
    directMessage.setSenderId(var["sender_id"].toLongLong());

    return directMessage;
}

/**
 *  Converts tweet list
 */
QTweetList QTweetConvert::variantMapToTweetList(const QVariantMap& var)
{
    QTweetList list;

    list.setMode(var["mode"].toString());
    list.setDescription(var["description"].toString());
    list.setFollowing(var["following"].toBool());
    list.setMemberCount(var["member_count"].toInt());
    list.setFullName(var["full_name"].toString());
    list.setSubscriberCount(var["subscriber_count"].toInt());
    list.setSlug(var["slug"].toString());
    list.setName(var["name"].toString());
    list.setId(var["id"].toLongLong());
    list.setUri(var["uri"].toString());

    if (var.contains("user")) {
        QVariantMap userMap = var["user"].toMap();

        QTweetUser user = variantMapToUserInfo(userMap);

        list.setUser(user);
    }
    return list;
}

/**
 *  Converts list of user infos
 */
QList<QTweetUser> QTweetConvert::variantToUserInfoList(const QVariant& fromParser)
{
    QList<QTweetUser> users;

    QList<QVariant> listUsers = fromParser.toList();

    foreach (const QVariant& user, listUsers) {
        QTweetUser userInfo = variantMapToUserInfo(user.toMap());

        users.append(userInfo);
    }

    return users;
}

/**
 *  Converts list of tweet lists
 */
QList<QTweetList> QTweetConvert::variantToTweetLists(const QVariant& var)
{
    QList<QTweetList> lists;

    QList<QVariant> varLists = var.toList();

    foreach (const QVariant& varlist, varLists) {
        QTweetList tweetlist = variantMapToTweetList(varlist.toMap());

        lists.append(tweetlist);
    }

    return lists;
}

/**
 *  Converts search result
 */
QTweetSearchResult QTweetConvert::variantMapToSearchResult(const QVariantMap& var)
{
    QTweetSearchResult result;

    result.setCreatedAt(var["created_at"].toString());
    result.setFromUser(var["from_user"].toString());
    result.setId(var["id"].toLongLong());
    result.setLang(var["iso_language_code"].toString());
    result.setProfileImageUrl(var["profile_image_url"].toString());
    result.setSource(var["source"].toString());
    result.setText(var["text"].toString());
    result.setToUser(var["to_user"].toString());

    return result;
}

/**
 *  Converts page results
 */
QTweetSearchPageResults QTweetConvert::variantToSearchPageResults(const QVariant& var)
{
    QTweetSearchPageResults page;

    QVariantMap varMap = var.toMap();

    page.setMaxId(varMap["max_id"].toLongLong());
    page.setNextPage(varMap["next_page"].toByteArray());
    page.setPage(varMap["page"].toInt());
    page.setQuery(varMap["query"].toByteArray());
    page.setRefreshUrl(varMap["refresh_url"].toByteArray());
    page.setResultsPerPage(varMap["results_per_page"].toInt());
    page.setSinceId(varMap["since_id"].toLongLong());
    page.setTotal(varMap["total"].toInt());

    QList<QTweetSearchResult> resultList;
    QList<QVariant> resultVarList = varMap["results"].toList();

    foreach (const QVariant& resultVar, resultVarList) {
        QTweetSearchResult result = variantMapToSearchResult(resultVar.toMap());
        resultList.append(result);
    }

    page.setResults(resultList);

    return page;
}

/**
 *  Converts place
 */
QTweetPlace QTweetConvert::variantMapToPlace(const QVariantMap& var)
{
    QTweetPlace place;

    place.setName(var["name"].toString());
    place.setCountryCode(var["country_code"].toString());
    place.setCountry(var["country"].toString());
    place.setID(var["id"].toString());
    place.setFullName(var["full_name"].toString());

    QString placeType = var["place_type"].toString();

    if (placeType == "poi")
        place.setType(QTweetPlace::Poi);
    else if (placeType == "neighborhood")
        place.setType(QTweetPlace::Neighborhood);
    else if (placeType == "city")
        place.setType(QTweetPlace::City);
    else if (placeType == "admin")
        place.setType(QTweetPlace::Admin);
    else if (placeType == "country")
        place.setType(QTweetPlace::Country);
    else
        place.setType(QTweetPlace::Neighborhood);   //twitter default

    QVariant bbVar = var["bounding_box"];

    if (!bbVar.isNull()) {
        QVariantMap bbMap = bbVar.toMap();

        if (bbMap["type"].toString() == "Polygon") {
            QVariantList coordList = bbMap["coordinates"].toList();

            if (coordList.count() == 1) {
                QVariantList latLongList = coordList.at(0).toList();

                if (latLongList.count() == 4) {
                    QTweetGeoBoundingBox box;

                    QVariantList coordsBottomLeft = latLongList.at(0).toList();
                    box.setBottomLeft(QTweetGeoCoord(coordsBottomLeft.at(1).toDouble(), coordsBottomLeft.at(0).toDouble()));

                    QVariantList coordsBottomRight = latLongList.at(1).toList();
                    box.setBottomRight(QTweetGeoCoord(coordsBottomRight.at(1).toDouble(), coordsBottomRight.at(0).toDouble()));

                    QVariantList coordsTopRight = latLongList.at(2).toList();
                    box.setTopRight(QTweetGeoCoord(coordsTopRight.at(1).toDouble(), coordsTopRight.at(0).toDouble()));

                    QVariantList coordsTopLeft = latLongList.at(3).toList();
                    box.setTopLeft(QTweetGeoCoord(coordsTopLeft.at(1).toDouble(), coordsTopLeft.at(0).toDouble()));

                    place.setBoundingBox(box);
                }
            }
        }
    }
    return place;
}

//not to be used in timelines api, but in geo api, where place contains other places
//is it recursive responsive?
QTweetPlace QTweetConvert::variantMapToPlaceRecursive(const QVariantMap& var)
{
    QTweetPlace place;

    place.setName(var["name"].toString());
    place.setCountryCode(var["country_code"].toString());
    place.setCountry(var["country"].toString());
    place.setID(var["id"].toString());
    place.setFullName(var["full_name"].toString());

    QString placeType = var["place_type"].toString();

    if (placeType == "poi")
        place.setType(QTweetPlace::Poi);
    else if (placeType == "neighborhood")
        place.setType(QTweetPlace::Neighborhood);
    else if (placeType == "city")
        place.setType(QTweetPlace::City);
    else if (placeType == "admin")
        place.setType(QTweetPlace::Admin);
    else if (placeType == "country")
        place.setType(QTweetPlace::Country);
    else
        place.setType(QTweetPlace::Neighborhood);   //twitter default

    QVariant bbVar = var["bounding_box"];

    if (!bbVar.isNull()) {
        QVariantMap bbMap = bbVar.toMap();

        if (bbMap["type"].toString() == "Polygon") {
            QVariantList coordList = bbMap["coordinates"].toList();

            if (coordList.count() == 1) {
                QVariantList latLongList = coordList.at(0).toList();

                if (latLongList.count() == 4) {
                    QTweetGeoBoundingBox box;

                    QVariantList coordsBottomLeft = latLongList.at(0).toList();
                    box.setBottomLeft(QTweetGeoCoord(coordsBottomLeft.at(1).toDouble(), coordsBottomLeft.at(0).toDouble()));

                    QVariantList coordsBottomRight = latLongList.at(1).toList();
                    box.setBottomRight(QTweetGeoCoord(coordsBottomRight.at(1).toDouble(), coordsBottomRight.at(0).toDouble()));

                    QVariantList coordsTopRight = latLongList.at(2).toList();
                    box.setTopRight(QTweetGeoCoord(coordsTopRight.at(1).toDouble(), coordsTopRight.at(0).toDouble()));

                    QVariantList coordsTopLeft = latLongList.at(3).toList();
                    box.setTopLeft(QTweetGeoCoord(coordsTopLeft.at(1).toDouble(), coordsTopLeft.at(0).toDouble()));

                    place.setBoundingBox(box);
                }
            }
        }
    }

    QVariantList containedVarList = var["contained_within"].toList();

    QList<QTweetPlace> containedInPlacesList;

    if (!containedVarList.isEmpty()) {
        foreach (const QVariant& containedVar, containedVarList) {
            QVariantMap containedPlaceMap = containedVar.toMap();

            QTweetPlace containedPlace = variantMapToPlaceRecursive(containedPlaceMap);

            containedInPlacesList.append(containedPlace);
        }
    }

    place.setContainedWithin(containedInPlacesList);

    return place;
}

/**
 *  Convers list of places
 */
QList<QTweetPlace> QTweetConvert::variantToPlaceList(const QVariant& fromParser)
{
    QList<QTweetPlace> placeList;

    QVariantMap responseVarMap = fromParser.toMap();
    QVariantMap resultVarMap = responseVarMap["result"].toMap();
    QVariantList placesVarList = resultVarMap["places"].toList();

    foreach (const QVariant& placeVar, placesVarList) {
        QTweetPlace place = variantMapToPlaceRecursive(placeVar.toMap());
        placeList.append(place);
    }

    return placeList;
}

QTweetEntityUrl QTweetConvert::variantMapToEntityUrl(const QVariantMap &var)
{
    QString url = var["url"].toString();
    QString displayUrl = var["display_url"].toString();
    QString expandedUrl = var["expanded_url"].toString();

    QTweetEntityUrl urlEntity;
    urlEntity.setUrl(url);
    urlEntity.setDisplayUrl(displayUrl);
    urlEntity.setExpandedUrl(expandedUrl);

    return urlEntity;
}

QTweetEntityHashtag QTweetConvert::variantMapToEntityHashtag(const QVariantMap &var)
{
    QTweetEntityHashtag hashtagEntity;
    hashtagEntity.setText(var["text"].toString());

    return hashtagEntity;
}

QTweetEntityUserMentions QTweetConvert::variantMapToEntityUserMentions(const QVariantMap &var)
{
    QTweetEntityUserMentions userMentionsEntity;
    userMentionsEntity.setScreenName(var["screen_name"].toString());
    userMentionsEntity.setName(var["name"].toString());
    userMentionsEntity.setUserid(var["id"].toLongLong());

    return userMentionsEntity;
}
