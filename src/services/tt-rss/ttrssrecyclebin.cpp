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

#include "services/tt-rss/ttrssrecyclebin.h"

#include "services/tt-rss/definitions.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"
#include "services/tt-rss/ttrssserviceroot.h"


TtRssRecycleBin::TtRssRecycleBin(RootItem* parent) : RecycleBin(parent) {
}

TtRssRecycleBin::~TtRssRecycleBin() {
}

TtRssServiceRoot* TtRssRecycleBin::serviceRoot() {
	return qobject_cast<TtRssServiceRoot*>(getParentServiceRoot());
}

bool TtRssRecycleBin::markAsReadUnread(RootItem::ReadStatus status) {
  serviceRoot()->network()->updateArticles(
      serviceRoot()->customIDSOfMessagesForItem(this),
      UpdateArticle::Unread, status == RootItem::Read ? UpdateArticle::SetToFalse : UpdateArticle::SetToTrue);
	return RecycleBin::markAsReadUnread(status);
}
