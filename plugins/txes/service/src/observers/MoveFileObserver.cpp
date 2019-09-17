/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/FileCache.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(CopyFile, model::CopyFileNotification<1>, [](const auto& notification, const ObserverContext& context) {
		auto& fileCache = context.Cache.sub<cache::FileCache>();
		auto destinationFileKey = state::MakeDriveFileKey(notification.Destination.Drive, notification.Destination.Hash);
		if (NotifyMode::Commit == context.Mode) {
			state::FileEntry destinationFileEntry(destinationFileKey);
			destinationFileEntry.setParentKey(state::MakeDriveFileKey(notification.Destination.Drive, notification.Destination.ParentHash));
			destinationFileEntry.setName(notification.Destination.Name);
			fileCache.insert(destinationFileEntry);
		} else {
			fileCache.remove(destinationFileKey);
		}
	});
}}
