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

#ifndef TTRSSNETWORKFACTORY_H
#define TTRSSNETWORKFACTORY_H

#include "core/message.h"

#include <QString>
#include <QPair>
#include <QNetworkReply>
#include <QJsonObject>


class RootItem;
class TtRssFeed;

class TtRssResponse {
	public:
		explicit TtRssResponse(const QString& raw_content = QString());
		virtual ~TtRssResponse();

		bool isLoaded() const;

		int seq() const;
		int status() const;
		QString error() const;
		bool hasError() const;
		bool isNotLoggedIn() const;
		QString toString() const;

	protected:
		QJsonObject m_rawContent;
};

class TtRssLoginResponse : public TtRssResponse {
	public:
		explicit TtRssLoginResponse(const QString& raw_content = QString());
		virtual ~TtRssLoginResponse();

		int apiLevel() const;
		QString sessionId() const;
};

class TtRssGetFeedsCategoriesResponse : public TtRssResponse {
	public:
		explicit TtRssGetFeedsCategoriesResponse(const QString& raw_content = QString());
		virtual ~TtRssGetFeedsCategoriesResponse();

		// Returns tree of feeds/categories.
		// Top-level root of the tree is not needed here.
		// Returned items do not have primary IDs assigned.
		RootItem* feedsCategories(bool obtain_icons, QString base_address = QString()) const;
};

class TtRssGetHeadlinesResponse : public TtRssResponse {
	public:
		explicit TtRssGetHeadlinesResponse(const QString& raw_content = QString());
		virtual ~TtRssGetHeadlinesResponse();

		QList<Message> messages() const;
};

class TtRssUpdateArticleResponse : public TtRssResponse {
	public:
		explicit TtRssUpdateArticleResponse(const QString& raw_content = QString());
		virtual ~TtRssUpdateArticleResponse();

		QString updateStatus() const;
		int articlesUpdated() const;
};

class TtRssSubscribeToFeedResponse : public TtRssResponse {
	public:
		explicit TtRssSubscribeToFeedResponse(const QString& raw_content = QString());
		virtual ~TtRssSubscribeToFeedResponse();

		int code() const;
};

class TtRssUnsubscribeFeedResponse : public TtRssResponse {
	public:
		explicit TtRssUnsubscribeFeedResponse(const QString& raw_content = QString());
		virtual ~TtRssUnsubscribeFeedResponse();

		QString code() const;
};

namespace UpdateArticle {
	enum Mode {
		SetToFalse  = 0,
		SetToTrue   = 1,
		Togggle     = 2
	};

	enum OperatingField {
		Starred     = 0,
		Published   = 1,
		Unread      = 2
	};
}

class TtRssNetworkFactory {
	public:
		explicit TtRssNetworkFactory();
		virtual ~TtRssNetworkFactory();

		QString url() const;
		void setUrl(const QString& url);

		QString username() const;
		void setUsername(const QString& username);

		QString password() const;
		void setPassword(const QString& password);

		bool authIsUsed() const;
		void setAuthIsUsed(bool auth_is_used);

		QString authUsername() const;
		void setAuthUsername(const QString& auth_username);

		QString authPassword() const;
		void setAuthPassword(const QString& auth_password);

		bool forceServerSideUpdate() const;
		void setForceServerSideUpdate(bool force_server_side_update);

		// Metadata.
		QDateTime lastLoginTime() const;
		QNetworkReply::NetworkError lastError() const;

		// Operations.

		// Logs user in.
		TtRssLoginResponse login();

		// Logs user out.
		TtRssResponse logout();

		// Gets feeds from the server.
		TtRssGetFeedsCategoriesResponse getFeedsCategories();

		// Gets headlines (messages) from the server.
		TtRssGetHeadlinesResponse getHeadlines(int feed_id, int limit, int skip,
		                                       bool show_content, bool include_attachments,
		                                       bool sanitize);

    void updateArticles(const QStringList& ids, UpdateArticle::OperatingField field, UpdateArticle::Mode mode);

		TtRssSubscribeToFeedResponse subscribeToFeed(const QString& url, int category_id, bool protectd = false,
		                                             const QString& username = QString(), const QString& password = QString());

		TtRssUnsubscribeFeedResponse unsubscribeFeed(int feed_id);

		//TtRssGetConfigResponse getConfig();

	private:
		QString m_bareUrl;
		QString m_fullUrl;
		QString m_username;
		QString m_password;
		bool m_forceServerSideUpdate;
		bool m_authIsUsed;
		QString m_authUsername;
		QString m_authPassword;
		QString m_sessionId;
		QDateTime m_lastLoginTime;
		QNetworkReply::NetworkError m_lastError;
};

#endif // TTRSSNETWORKFACTORY_H
