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

#ifndef OWNCLOUDNETWORKFACTORY_H
#define OWNCLOUDNETWORKFACTORY_H

#include "core/message.h"
#include "services/abstract/rootitem.h"

#include <QDateTime>
#include <QString>
#include <QIcon>
#include <QNetworkReply>
#include <QJsonObject>


class OwnCloudResponse {
	public:
		explicit OwnCloudResponse(const QString& raw_content = QString());
		virtual ~OwnCloudResponse();

		bool isLoaded() const;
		QString toString() const;

	protected:
		QJsonObject m_rawContent;
		bool m_emptyString;
};

class OwnCloudUserResponse : public OwnCloudResponse {
	public:
		explicit OwnCloudUserResponse(const QString& raw_content = QString());
		virtual ~OwnCloudUserResponse();

		QString userId() const;
		QString displayName() const;
		QDateTime lastLoginTime() const;
		QIcon avatar() const;
};

class OwnCloudGetMessagesResponse : public OwnCloudResponse {
	public:
		explicit OwnCloudGetMessagesResponse(const QString& raw_content = QString());
		virtual ~OwnCloudGetMessagesResponse();

		QList<Message> messages() const;
};

class OwnCloudStatusResponse : public OwnCloudResponse {
	public:
		explicit OwnCloudStatusResponse(const QString& raw_content = QString());
		virtual ~OwnCloudStatusResponse();

		QString version() const;
		bool misconfiguredCron() const;
};

class RootItem;

class OwnCloudGetFeedsCategoriesResponse {
	public:
		explicit OwnCloudGetFeedsCategoriesResponse(const QString& raw_categories = QString(),
		                                            const QString& raw_feeds = QString());
		virtual ~OwnCloudGetFeedsCategoriesResponse();

		// Returns tree of feeds/categories.
		// Top-level root of the tree is not needed here.
		// Returned items do not have primary IDs assigned.
		RootItem* feedsCategories(bool obtain_icons) const;

	private:
		QString m_contentCategories;
		QString m_contentFeeds;
};

class OwnCloudNetworkFactory {
	public:
		explicit OwnCloudNetworkFactory();
		virtual ~OwnCloudNetworkFactory();

		QString url() const;
		void setUrl(const QString& url);

		bool forceServerSideUpdate() const;
		void setForceServerSideUpdate(bool force_update);

		QString authUsername() const;
		void setAuthUsername(const QString& auth_username);

		QString authPassword() const;
		void setAuthPassword(const QString& auth_password);

		QString userId() const;
		void setUserId(const QString& userId);

		QNetworkReply::NetworkError lastError() const;

		// Operations.

		// Get user info.
		OwnCloudUserResponse userInfo();

		// Get version info.
		OwnCloudStatusResponse status();

		// Get feeds & categories (used for sync-in).
		OwnCloudGetFeedsCategoriesResponse feedsCategories();

		// Feed operations.
		bool deleteFeed(int feed_id);
		bool createFeed(const QString& url, int parent_id);
		bool renameFeed(const QString& new_name, int feed_id);

		// Get messages for given feed.
		OwnCloudGetMessagesResponse getMessages(int feed_id);

		// Misc methods.
		QNetworkReply::NetworkError triggerFeedUpdate(int feed_id);
    void markMessagesRead(RootItem::ReadStatus status, const QStringList& custom_ids);
    void markMessagesStarred(RootItem::Importance importance, const QStringList& feed_ids,
                             const QStringList& guid_hashes);

    // Gets/sets the amount of messages to obtain during single feed update.
    int batchSize() const;
    void setBatchSize(int batch_size);

  private:
    QString m_url;
    QString m_fixedUrl;
    bool m_forceServerSideUpdate;
		QString m_authUsername;
		QString m_authPassword;
		QNetworkReply::NetworkError m_lastError;
    int m_batchSize;

		// Endpoints.
		QString m_urlUser;
		QString m_urlStatus;
		QString m_urlFolders;
		QString m_urlFeeds;
		QString m_urlMessages;
		QString m_urlFeedsUpdate;
		QString m_urlDeleteFeed;
		QString m_urlRenameFeed;
		QString m_userId;
};

#endif // OWNCLOUDNETWORKFACTORY_H
