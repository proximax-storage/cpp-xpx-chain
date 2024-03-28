/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "ObserverContext.h"

namespace catapult::observers {

class StorageExternalManagementObserver {

public:

	virtual ~StorageExternalManagementObserver() = default;

	virtual void updateStorageState(
			const ObserverContext& context,
			const Key& driveKey,
			const Hash256& storageHash,
			const Hash256& modificationId,
			uint64_t usedSize,
			uint64_t metaFilesSize) const = 0;

	virtual void addToConfirmedStorage(
			const ObserverContext& context,
			const Key& driveKey,
			const std::set<Key>& replicators) const = 0;

	virtual void allowOwnerManagement(const ObserverContext& context,
									  const Key& driveKey) const = 0;
};

}