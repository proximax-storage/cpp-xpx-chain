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
#include <stdint.h>

namespace catapult { namespace cache {

	/// Cache ids for well-known caches.
	enum class CacheId : uint32_t {
		AccountState,
		BlockDifficulty,
		Hash,
		Namespace,
		Mosaic,
		Multisig,
		HashLockInfo,
		SecretLockInfo,
		Property
	};

/// Defines cache constants for a cache with \a NAME.
#define DEFINE_CACHE_CONSTANTS(NAME) \
	static constexpr size_t Id = utils::to_underlying_type(CacheId::NAME); \
	static constexpr auto Name = #NAME "Cache";
}}
