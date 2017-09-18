// This file is part of RSS Guard.
//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "services/tt-rss/network/ttrssnetworkfactory.h"

#include "definitions/definitions.h"
#include "services/abstract/rootitem.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/ttrssfeed.h"
#include "services/tt-rss/ttrsscategory.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "network-web/networkfactory.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QPair>
#include <QVariant>


TtRssNetworkFactory::TtRssNetworkFactory()
	: m_bareUrl(QString()), m_fullUrl(QString()), m_username(QString()), m_password(QString()), m_forceServerSideUpdate(false),
	  m_authIsUsed(false),
	  m_authUsername(QString()), m_authPassword(QString()), m_sessionId(QString()),
	  m_lastLoginTime(QDateTime()), m_lastError(QNetworkReply::NoError) {
}

TtRssNetworkFactory::~TtRssNetworkFactory() {
}

QString TtRssNetworkFactory::url() const {
	return m_bareUrl;
}

void TtRssNetworkFactory::setUrl(const QString& url) {
	m_bareUrl = url;

	if (!m_bareUrl.endsWith(QSL("/"))) {
		m_bareUrl = m_bareUrl + QSL("/");
	}

	if (!m_bareUrl.endsWith(QSL("api/"))) {
		m_fullUrl = m_bareUrl + QSL("api/");
	}
	else {
		m_fullUrl = m_bareUrl;
	}
}

QString TtRssNetworkFactory::username() const {
	return m_username;
}

void TtRssNetworkFactory::setUsername(const QString& username) {
	m_username = username;
}

QString TtRssNetworkFactory::password() const {
	return m_password;
}

void TtRssNetworkFactory::setPassword(const QString& password) {
	m_password = password;
}

QDateTime TtRssNetworkFactory::lastLoginTime() const {
	return m_lastLoginTime;
}

QNetworkReply::NetworkError TtRssNetworkFactory::lastError() const {
	return m_lastError;
}

TtRssLoginResponse TtRssNetworkFactory::login() {
	if (!m_sessionId.isEmpty()) {
		qDebug("TT-RSS: Session ID is not empty before login, logging out first.");
		logout();
	}

	QJsonObject json;
	json["op"] = QSL("login");
	json["user"] = m_username;
	json["password"] = m_password;
	QByteArray result_raw;
	NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, qApp->settings()->value(GROUP(Feeds),
	                              SETTING(Feeds::UpdateTimeout)).toInt(),
	                              QJsonDocument(json).toJson(QJsonDocument::Compact), CONTENT_TYPE, result_raw,
	                              QNetworkAccessManager::PostOperation,
	                              m_authIsUsed, m_authUsername, m_authPassword);
	TtRssLoginResponse login_response(QString::fromUtf8(result_raw));

	if (network_reply.first == QNetworkReply::NoError) {
		m_sessionId = login_response.sessionId();
		m_lastLoginTime = QDateTime::currentDateTime();
	}
	else {
		qWarning("TT-RSS: Login failed with error %d.", network_reply.first);
	}

	m_lastError = network_reply.first;
	return login_response;
}

TtRssResponse TtRssNetworkFactory::logout() {
	if (!m_sessionId.isEmpty()) {
		QJsonObject json;
		json["op"] = QSL("logout");
		json["sid"] = m_sessionId;
		QByteArray result_raw;
		NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, qApp->settings()->value(GROUP(Feeds),
		                              SETTING(Feeds::UpdateTimeout)).toInt(),
		                              QJsonDocument(json).toJson(QJsonDocument::Compact), CONTENT_TYPE, result_raw,
		                              QNetworkAccessManager::PostOperation,
		                              m_authIsUsed, m_authUsername, m_authPassword);
		m_lastError = network_reply.first;

		if (m_lastError == QNetworkReply::NoError) {
			m_sessionId.clear();
		}
		else {
			qWarning("TT-RSS: Logout failed with error %d.", network_reply.first);
		}

		return TtRssResponse(QString::fromUtf8(result_raw));
	}
	else {
		qWarning("TT-RSS: Cannot logout because session ID is empty.");
		m_lastError = QNetworkReply::NoError;
		return TtRssResponse();
	}
}

TtRssGetFeedsCategoriesResponse TtRssNetworkFactory::getFeedsCategories() {
	QJsonObject json;
	json["op"] = QSL("getFeedTree");
	json["sid"] = m_sessionId;
	json["include_empty"] = true;
	const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
	QByteArray result_raw;
	NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout,
	                              QJsonDocument(json).toJson(QJsonDocument::Compact),
	                              CONTENT_TYPE, result_raw,
	                              QNetworkAccessManager::PostOperation,
	                              m_authIsUsed, m_authUsername, m_authPassword);
	TtRssGetFeedsCategoriesResponse result(QString::fromUtf8(result_raw));

	if (result.isNotLoggedIn()) {
		// We are not logged in.
		login();
		json["sid"] = m_sessionId;
		network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout, QJsonDocument(json).toJson(QJsonDocument::Compact),
		                                                        CONTENT_TYPE, result_raw,
		                                                        QNetworkAccessManager::PostOperation,
		                                                        m_authIsUsed, m_authUsername, m_authPassword);
		result = TtRssGetFeedsCategoriesResponse(QString::fromUtf8(result_raw));
	}

	if (network_reply.first != QNetworkReply::NoError) {
		qWarning("TT-RSS: getFeedTree failed with error %d.", network_reply.first);
	}

	m_lastError = network_reply.first;
	return result;
}

TtRssGetHeadlinesResponse TtRssNetworkFactory::getHeadlines(int feed_id, int limit, int skip,
                                                            bool show_content, bool include_attachments,
                                                            bool sanitize) {
	QJsonObject json;
	json["op"] = QSL("getHeadlines");
	json["sid"] = m_sessionId;
	json["feed_id"] = feed_id;
	json["force_update"] = m_forceServerSideUpdate;
	json["limit"] = limit;
	json["skip"] = skip;
	json["show_content"] = show_content;
	json["include_attachments"] = include_attachments;
	json["sanitize"] = sanitize;
	const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
	QByteArray result_raw;
	NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout,
	                              QJsonDocument(json).toJson(QJsonDocument::Compact),
	                              CONTENT_TYPE, result_raw,
	                              QNetworkAccessManager::PostOperation,
	                              m_authIsUsed, m_authUsername, m_authPassword);
	TtRssGetHeadlinesResponse result(QString::fromUtf8(result_raw));

	if (result.isNotLoggedIn()) {
		// We are not logged in.
		login();
		json["sid"] = m_sessionId;
		network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout, QJsonDocument(json).toJson(QJsonDocument::Compact),
		                                                        CONTENT_TYPE, result_raw,
		                                                        QNetworkAccessManager::PostOperation,
		                                                        m_authIsUsed, m_authUsername, m_authPassword);
		result = TtRssGetHeadlinesResponse(QString::fromUtf8(result_raw));
	}

	IOFactory::writeTextFile("aaa", result_raw);

	if (network_reply.first != QNetworkReply::NoError) {
		qWarning("TT-RSS: getHeadlines failed with error %d.", network_reply.first);
	}

	m_lastError = network_reply.first;
	return result;
}

void TtRssNetworkFactory::updateArticles(const QStringList& ids,
                                         UpdateArticle::OperatingField field,
                                         UpdateArticle::Mode mode) {
	QJsonObject json;
	json["op"] = QSL("updateArticle");
	json["sid"] = m_sessionId;
	json["article_ids"] = ids.join(QSL(","));
	json["mode"] = (int) mode;
	json["field"] = (int) field;
	const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  Downloader* downloader = NetworkFactory::performAsyncNetworkOperation(
                               m_fullUrl, timeout,
                               QJsonDocument(json).toJson(QJsonDocument::Compact),
                               CONTENT_TYPE,
                               QNetworkAccessManager::PostOperation,
                               m_authIsUsed, m_authUsername, m_authPassword);

  QObject::connect(downloader, &Downloader::completed, [ = ](QNetworkReply::NetworkError status, QByteArray data) {
    downloader->deleteLater();

    TtRssUpdateArticleResponse result(QString::fromUtf8(data));

    if (status != QNetworkReply::NoError) {
      qWarning("TT-RSS: updateArticle failed with error %d.", status);
    }

    if (result.isNotLoggedIn()) {
      // We are not logged in.
      login();
      json["sid"] = m_sessionId;
      Downloader* next_downloader = NetworkFactory::performAsyncNetworkOperation(
                                        m_fullUrl, timeout, QJsonDocument(json).toJson(QJsonDocument::Compact),
                                        CONTENT_TYPE,
                                        QNetworkAccessManager::PostOperation,
                                        m_authIsUsed, m_authUsername, m_authPassword);
      QObject::connect(next_downloader, &Downloader::completed, next_downloader, &Downloader::deleteLater);
    }
  });
}

TtRssSubscribeToFeedResponse TtRssNetworkFactory::subscribeToFeed(const QString& url, int category_id,
        bool protectd, const QString& username,
        const QString& password) {
	QJsonObject json;
	json["op"] = QSL("subscribeToFeed");
	json["sid"] = m_sessionId;
	json["feed_url"] = url;
	json["category_id"] = category_id;

	if (protectd) {
		json["login"] = username;
		json["password"] = password;
	}

	const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
	QByteArray result_raw;
	NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout,
	                              QJsonDocument(json).toJson(QJsonDocument::Compact),
	                              CONTENT_TYPE, result_raw,
	                              QNetworkAccessManager::PostOperation,
	                              m_authIsUsed, m_authUsername, m_authPassword);
	TtRssSubscribeToFeedResponse result(QString::fromUtf8(result_raw));

	if (result.isNotLoggedIn()) {
		// We are not logged in.
		login();
		json["sid"] = m_sessionId;
		network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout, QJsonDocument(json).toJson(QJsonDocument::Compact),
		                                                        CONTENT_TYPE, result_raw,
		                                                        QNetworkAccessManager::PostOperation,
		                                                        m_authIsUsed, m_authUsername, m_authPassword);
		result = TtRssSubscribeToFeedResponse(QString::fromUtf8(result_raw));
	}

	if (network_reply.first != QNetworkReply::NoError) {
		qWarning("TT-RSS: updateArticle failed with error %d.", network_reply.first);
	}

	m_lastError = network_reply.first;
	return result;
}

TtRssUnsubscribeFeedResponse TtRssNetworkFactory::unsubscribeFeed(int feed_id) {
	QJsonObject json;
	json["op"] = QSL("unsubscribeFeed");
	json["sid"] = m_sessionId;
	json["feed_id"] = feed_id;
	const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
	QByteArray result_raw;
	NetworkResult network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout,
	                              QJsonDocument(json).toJson(QJsonDocument::Compact), CONTENT_TYPE, result_raw,
	                              QNetworkAccessManager::PostOperation,
	                              m_authIsUsed, m_authUsername, m_authPassword);
	TtRssUnsubscribeFeedResponse result(QString::fromUtf8(result_raw));

	if (result.isNotLoggedIn()) {
		// We are not logged in.
		login();
		json["sid"] = m_sessionId;
		network_reply = NetworkFactory::performNetworkOperation(m_fullUrl, timeout, QJsonDocument(json).toJson(QJsonDocument::Compact),
		                                                        CONTENT_TYPE, result_raw,
		                                                        QNetworkAccessManager::PostOperation,
		                                                        m_authIsUsed, m_authUsername, m_authPassword);
		result = TtRssUnsubscribeFeedResponse(QString::fromUtf8(result_raw));
	}

	if (network_reply.first != QNetworkReply::NoError) {
		qWarning("TT-RSS: getFeeds failed with error %d.", network_reply.first);
	}

	m_lastError = network_reply.first;
	return result;
}

bool TtRssNetworkFactory::forceServerSideUpdate() const {
	return m_forceServerSideUpdate;
}

void TtRssNetworkFactory::setForceServerSideUpdate(bool force_server_side_update) {
	m_forceServerSideUpdate = force_server_side_update;
}

bool TtRssNetworkFactory::authIsUsed() const {
	return m_authIsUsed;
}

void TtRssNetworkFactory::setAuthIsUsed(bool auth_is_used) {
	m_authIsUsed = auth_is_used;
}

QString TtRssNetworkFactory::authUsername() const {
	return m_authUsername;
}

void TtRssNetworkFactory::setAuthUsername(const QString& auth_username) {
	m_authUsername = auth_username;
}

QString TtRssNetworkFactory::authPassword() const {
	return m_authPassword;
}

void TtRssNetworkFactory::setAuthPassword(const QString& auth_password) {
	m_authPassword = auth_password;
}

TtRssResponse::TtRssResponse(const QString& raw_content) {
	m_rawContent = QJsonDocument::fromJson(raw_content.toUtf8()).object();
}

TtRssResponse::~TtRssResponse() {
}

bool TtRssResponse::isLoaded() const {
	return !m_rawContent.isEmpty();
}

int TtRssResponse::seq() const {
	if (!isLoaded()) {
		return CONTENT_NOT_LOADED;
	}
	else {
		return m_rawContent["seq"].toInt();
	}
}

int TtRssResponse::status() const {
	if (!isLoaded()) {
		return CONTENT_NOT_LOADED;
	}
	else {
		return m_rawContent["status"].toInt();
	}
}

bool TtRssResponse::isNotLoggedIn() const {
	return status() == API_STATUS_ERR && hasError() && error() == NOT_LOGGED_IN;
}

QString TtRssResponse::toString() const {
	return QJsonDocument(m_rawContent).toJson(QJsonDocument::Compact);
}

TtRssLoginResponse::TtRssLoginResponse(const QString& raw_content) : TtRssResponse(raw_content) {
}

TtRssLoginResponse::~TtRssLoginResponse() {
}

int TtRssLoginResponse::apiLevel() const {
	if (!isLoaded()) {
		return CONTENT_NOT_LOADED;
	}
	else {
		return m_rawContent["content"].toObject()["api_level"].toInt();
	}
}

QString TtRssLoginResponse::sessionId() const {
	if (!isLoaded()) {
		return QString();
	}
	else {
		return m_rawContent["content"].toObject()["session_id"].toString();
	}
}

QString TtRssResponse::error() const {
	if (!isLoaded()) {
		return QString();
	}
	else {
		return m_rawContent["content"].toObject()["error"].toString();
	}
}

bool TtRssResponse::hasError() const {
	if (!isLoaded()) {
		return false;
	}
	else {
		return m_rawContent["content"].toObject().contains("error");
	}
}


TtRssGetFeedsCategoriesResponse::TtRssGetFeedsCategoriesResponse(const QString& raw_content) : TtRssResponse(raw_content) {
}

TtRssGetFeedsCategoriesResponse::~TtRssGetFeedsCategoriesResponse() {
}

RootItem* TtRssGetFeedsCategoriesResponse::feedsCategories(bool obtain_icons, QString base_address) const {
	RootItem* parent = new RootItem();
	// Chop the "api/" from the end of the address.
	base_address.chop(4);
	qDebug("TT-RSS: Chopped base address to '%s' to get feed icons.", qPrintable(base_address));

	if (status() == API_STATUS_OK) {
		// We have data, construct object tree according to data.
		QJsonArray items_to_process = m_rawContent["content"].toObject()["categories"].toObject()["items"].toArray();
		QVector<QPair<RootItem*, QJsonValue>> pairs;

		foreach (const QJsonValue& item, items_to_process) {
			pairs.append(QPair<RootItem*, QJsonValue>(parent, item));
		}

		while (!pairs.isEmpty()) {
			QPair<RootItem*, QJsonValue> pair = pairs.takeFirst();
			RootItem* act_parent = pair.first;
			QJsonObject item = pair.second.toObject();
			int item_id = item["bare_id"].toInt();
			bool is_category = item.contains("type") && item["type"].toString() == GFT_TYPE_CATEGORY;

			if (item_id >= 0) {
				if (is_category) {
					if (item_id == 0) {
						// This is "Uncategorized" category, all its feeds belong to top-level root.
						if (item.contains("items")) {
							foreach (const QJsonValue& child_feed, item["items"].toArray()) {
								pairs.append(QPair<RootItem*, QJsonValue>(parent, child_feed));
							}
						}
					}
					else {
						TtRssCategory* category = new TtRssCategory();
						category->setTitle(item["name"].toString());
						category->setCustomId(item_id);
						act_parent->appendChild(category);

						if (item.contains("items")) {
							foreach (const QJsonValue& child, item["items"].toArray()) {
								pairs.append(QPair<RootItem*, QJsonValue>(category, child));
							}
						}
					}
				}
				else {
					// We have feed.
					TtRssFeed* feed = new TtRssFeed();

					if (obtain_icons) {
						QString icon_path = item["icon"].type() == QJsonValue::String ? item["icon"].toString() : QString();

						if (!icon_path.isEmpty()) {
							// Chop the "api/" suffix out and append
							QString full_icon_address = base_address + QL1C('/') + icon_path;
							QByteArray icon_data;

							if (NetworkFactory::performNetworkOperation(full_icon_address, DOWNLOAD_TIMEOUT,
							                                            QByteArray(), QString(), icon_data,
							                                            QNetworkAccessManager::GetOperation).first == QNetworkReply::NoError) {
								// Icon downloaded, set it up.
								QPixmap icon_pixmap;
								icon_pixmap.loadFromData(icon_data);
								feed->setIcon(QIcon(icon_pixmap));
							}
						}
					}

					feed->setTitle(item["name"].toString());
					feed->setCustomId(item_id);
					act_parent->appendChild(feed);
				}
			}
		}
	}

	return parent;
}


TtRssGetHeadlinesResponse::TtRssGetHeadlinesResponse(const QString& raw_content) : TtRssResponse(raw_content) {
}

TtRssGetHeadlinesResponse::~TtRssGetHeadlinesResponse() {
}

QList<Message> TtRssGetHeadlinesResponse::messages() const {
	QList<Message> messages;

	foreach (const QJsonValue& item, m_rawContent["content"].toArray()) {
		QJsonObject mapped = item.toObject();
		Message message;
		message.m_author = mapped["author"].toString();
		message.m_isRead = !mapped["unread"].toBool();
		message.m_isImportant = mapped["marked"].toBool();
		message.m_contents = mapped["content"].toString();
		// Multiply by 1000 because Tiny Tiny RSS API does not include miliseconds in Unix
		// date/time number.
		message.m_created = TextFactory::parseDateTime(mapped["updated"].toDouble() * 1000);
		message.m_createdFromFeed = true;
		message.m_customId = QString::number(mapped["id"].toInt());
		message.m_feedId = mapped["feed_id"].toString();
		message.m_title = mapped["title"].toString();
		message.m_url = mapped["link"].toString();

		if (mapped.contains(QSL("attachments"))) {
			// Process enclosures.
			foreach (const QJsonValue& attachment, mapped["attachments"].toArray()) {
				QJsonObject mapped_attachemnt = attachment.toObject();
				Enclosure enclosure;
				enclosure.m_mimeType = mapped_attachemnt["content_type"].toString();
				enclosure.m_url = mapped_attachemnt["content_url"].toString();
				message.m_enclosures.append(enclosure);
			}
		}

		messages.append(message);
	}

	return messages;
}


TtRssUpdateArticleResponse::TtRssUpdateArticleResponse(const QString& raw_content) : TtRssResponse(raw_content) {
}

TtRssUpdateArticleResponse::~TtRssUpdateArticleResponse() {
}

QString TtRssUpdateArticleResponse::updateStatus() const {
	if (m_rawContent.contains(QSL("content"))) {
		return m_rawContent["content"].toObject()["status"].toString();
	}
	else {
		return QString();
	}
}

int TtRssUpdateArticleResponse::articlesUpdated() const {
	if (m_rawContent.contains(QSL("content"))) {
		return m_rawContent["content"].toObject()["updated"].toInt();
	}
	else {
		return 0;
	}
}

TtRssSubscribeToFeedResponse::TtRssSubscribeToFeedResponse(const QString& raw_content) : TtRssResponse(raw_content) {
}

TtRssSubscribeToFeedResponse::~TtRssSubscribeToFeedResponse() {
}

int TtRssSubscribeToFeedResponse::code() const {
	if (m_rawContent.contains(QSL("content"))) {
		return m_rawContent["content"].toObject()["status"].toObject()["code"].toInt();
	}
	else {
		return STF_UNKNOWN;
	}
}


TtRssUnsubscribeFeedResponse::TtRssUnsubscribeFeedResponse(const QString& raw_content) : TtRssResponse(raw_content) {
}

TtRssUnsubscribeFeedResponse::~TtRssUnsubscribeFeedResponse() {
}

QString TtRssUnsubscribeFeedResponse::code() const {
	if (m_rawContent.contains(QSL("content"))) {
		QJsonObject map = m_rawContent["content"].toObject();

		if (map.contains(QSL("error"))) {
			return map["error"].toString();
		}
		else if (map.contains(QSL("status"))) {
			return map["status"].toString();
		}
	}

	return QString();
}
