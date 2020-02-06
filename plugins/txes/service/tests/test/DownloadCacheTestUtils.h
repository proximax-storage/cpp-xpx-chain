/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/cache/DownloadCache.h"
#include "src/cache/DownloadCacheStorage.h"
#include "src/cache/DownloadCacheTypes.h"
#include "plugins/txes/lock_shared/tests/test/LockInfoCacheTestUtils.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	/// Basic traits for a secret lock info.
	struct BasicDownloadTestTraits : public cache::DownloadCacheDescriptor {
		using cache::DownloadCacheDescriptor::ValueType;

		static constexpr auto ToKey = cache::DownloadCacheDescriptor::GetKeyFromValue;

		/// Creates a secret lock info with given \a height and \a status.
		static ValueType CreateLockInfo(Height height, state::LockStatus status = state::LockStatus::Unused);

		/// Creates a random secret lock info.
		static ValueType CreateLockInfo();

		/// Sets the \a key of the \a lockInfo.
		static void SetKey(ValueType& lockInfo, const KeyType& key);

		/// Asserts that the secret lock infos \a lhs and \a rhs are equal.
		static void AssertEqual(const ValueType& lhs, const ValueType& rhs);
	};
}}
