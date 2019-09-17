/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/FileCache.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(RemoveDirectory, model::RemoveDirectoryNotification<1>, [](const auto& notification, const ObserverContext& context) {
		auto& fileCache = context.Cache.sub<cache::FileCache>();
		auto fileKey = state::MakeDriveFileKey(notification.File.Drive, notification.File.Hash);
		if (NotifyMode::Commit == context.Mode) {
			fileCache.remove(fileKey);
		} else {
			state::FileEntry fileEntry(fileKey);
			fileEntry.setParentKey(state::MakeDriveFileKey(notification.File.Drive, notification.File.ParentHash));
			fileEntry.setName(notification.File.Name);
			fileCache.insert(fileEntry);
		}
	});
}}
