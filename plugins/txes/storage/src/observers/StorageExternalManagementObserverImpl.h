/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/observers/StorageExternalManagementObserver.h"

namespace catapult::observers {

class StorageExternalManagementObserverImpl: public StorageExternalManagementObserver {

public:

	void updateStorageState(
			const ObserverContext& context,
			const Key& driveKey,
			const Hash256& storageHash,
			const Hash256& modificationId,
			uint64_t usedSize,
			uint64_t metaFilesSize) override;

	void addToConfirmedStorage(const ObserverContext& context,
							   const Key& driveKey,
							   const std::set<Key>& replicators) override;

	void allowOwnerManagement(const ObserverContext& context, const Key& driveKey) override;
};

}