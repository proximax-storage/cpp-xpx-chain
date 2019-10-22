/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ServiceUtils.h"
#include <map>

namespace catapult { namespace utils {

	Amount CalculateDriveDeposit(const state::DriveEntry& driveEntry) {
		return Amount(driveEntry.size());
	}

	Amount CalculateFileDeposit(const state::DriveEntry& driveEntry, const Hash256& fileHash) {
		return Amount(driveEntry.files().find(fileHash)->second.Size);
	}

	Amount CalculateFileUpload(const state::DriveEntry& driveEntry, const uint64_t& size) {
		return Amount((driveEntry.replicas() - 1) * size);
	}

}}
