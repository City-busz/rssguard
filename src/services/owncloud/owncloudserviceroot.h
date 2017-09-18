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

#ifndef OWNCLOUDSERVICEROOT_H
#define OWNCLOUDSERVICEROOT_H

#include "services/abstract/serviceroot.h"

#include <QMap>


class OwnCloudNetworkFactory;
class OwnCloudRecycleBin;
class Mutex;

class OwnCloudServiceRoot : public ServiceRoot {
		Q_OBJECT

	public:
		explicit OwnCloudServiceRoot(RootItem* parent = nullptr);
		virtual ~OwnCloudServiceRoot();

		bool canBeEdited() const;
		bool canBeDeleted() const;
		bool editViaGui();
		bool deleteViaGui();
		bool supportsFeedAdding() const;
		bool supportsCategoryAdding() const;
		QList<QAction*> serviceMenu();
		RecycleBin* recycleBin() const;
		void start(bool freshly_activated);
		void stop();
		QString code() const;
		bool markAsReadUnread(ReadStatus status);

		OwnCloudNetworkFactory* network() const;

		bool onBeforeSetMessagesRead(RootItem* selected_item, const QList<Message>& messages, ReadStatus read);
		bool onBeforeSwitchMessageImportance(RootItem* selected_item, const QList<ImportanceChange>& changes);

		void updateTitle();
		void saveAccountDataToDatabase();

	public slots:
		void addNewFeed(const QString& url);
		void addNewCategory();

	private:
		QMap<int, QVariant> storeCustomFeedsData();
		void restoreCustomFeedsData(const QMap<int, QVariant>& data, const QHash<int, Feed*>& feeds);
		RootItem* obtainNewTreeForSyncIn() const;

		void loadFromDatabase();

		OwnCloudRecycleBin* m_recycleBin;
		QAction* m_actionSyncIn;
		QList<QAction*> m_serviceMenu;
		OwnCloudNetworkFactory* m_network;
};

#endif // OWNCLOUDSERVICEROOT_H
