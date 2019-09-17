/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/FileCache.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(MoveFile, model::MoveFileNotification<1>, [](const auto& notification, const ObserverContext& context) {
		auto& fileCache = context.Cache.sub<cache::FileCache>();
		auto& fileEntry = fileCache.find(state::MakeDriveFileKey(notification.Source.Drive, notification.Source.Hash)).get();
		if (NotifyMode::Commit == context.Mode) {
			fileEntry.setParentKey(state::MakeDriveFileKey(notification.Destination.Drive, notification.Destination.ParentHash));
			fileEntry.setName(notification.Destination.Name);
		} else {
			fileEntry.setParentKey(state::MakeDriveFileKey(notification.Source.Drive, notification.Source.ParentHash));
			fileEntry.setName(notification.Source.Name);
		}
	});
}}
