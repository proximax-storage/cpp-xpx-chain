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
#include "HashLockInfoBaseSets.h"
#include "plugins/txes/lock_shared/src/cache/LockInfoCacheDelta.h"

namespace catapult { namespace cache {

	/// Basic delta on top of the hash lock info cache.
	class BasicHashLockInfoCacheDelta
		: public BasicLockInfoCacheDelta<HashLockInfoCacheDescriptor, HashLockInfoCacheTypes>
		, public LockInfoCacheDeltaMixins<HashLockInfoCacheDescriptor, HashLockInfoCacheTypes>::Enable
		, public LockInfoCacheDeltaMixins<HashLockInfoCacheDescriptor, HashLockInfoCacheTypes>::Height {
	public:
		using BasicLockInfoCacheDelta<HashLockInfoCacheDescriptor, HashLockInfoCacheTypes>::BasicLockInfoCacheDelta;
	};

	/// Delta on top of the hash lock info cache.
	class HashLockInfoCacheDelta
			: public LockInfoCacheDelta<HashLockInfoCacheDescriptor, HashLockInfoCacheTypes, BasicHashLockInfoCacheDelta> {
	public:
		using LockInfoCacheDelta<HashLockInfoCacheDescriptor, HashLockInfoCacheTypes, BasicHashLockInfoCacheDelta>::LockInfoCacheDelta;
	};
}}
