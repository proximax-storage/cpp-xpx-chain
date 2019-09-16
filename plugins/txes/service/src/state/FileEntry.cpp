/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "FileEntry.h"

namespace catapult { namespace state {

	DriveFileKey MakeDriveFileKey(const Key& drive, const Hash256& fileHash) {
		DriveFileKey driveFileKey;
		memcpy(driveFileKey.data(), drive.data(), drive.size());
		memcpy(driveFileKey.data() + Key_Size, fileHash.data(), fileHash.size());
		return driveFileKey;
	}
}}