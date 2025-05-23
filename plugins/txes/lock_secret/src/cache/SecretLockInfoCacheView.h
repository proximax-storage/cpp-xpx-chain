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
#include "SecretLockInfoBaseSets.h"
#include "plugins/txes/lock_shared/src/cache/LockInfoCacheView.h"

namespace catapult { namespace cache {

	/// Basic view on top of the secret lock info cache.
	class BasicSecretLockInfoCacheView
		: public BasicLockInfoCacheView<SecretLockInfoCacheDescriptor, SecretLockInfoCacheTypes>
		, public LockInfoCacheViewMixins<SecretLockInfoCacheDescriptor, SecretLockInfoCacheTypes>::Enable
		, public LockInfoCacheViewMixins<SecretLockInfoCacheDescriptor, SecretLockInfoCacheTypes>::Height {
	public:
		using BasicLockInfoCacheView<SecretLockInfoCacheDescriptor, SecretLockInfoCacheTypes>::BasicLockInfoCacheView;
	};

	/// View on top of the secret lock info cache.
	class SecretLockInfoCacheView
			: public LockInfoCacheView<SecretLockInfoCacheDescriptor, SecretLockInfoCacheTypes, BasicSecretLockInfoCacheView> {
	public:
		using LockInfoCacheView<SecretLockInfoCacheDescriptor, SecretLockInfoCacheTypes, BasicSecretLockInfoCacheView>::LockInfoCacheView;
	};
}}
