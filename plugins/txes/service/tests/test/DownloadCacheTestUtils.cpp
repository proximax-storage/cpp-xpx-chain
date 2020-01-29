/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DownloadCacheTestUtils.h"
#include "ServiceTestUtils.h"

namespace catapult { namespace test {

	BasicDownloadTestTraits::ValueType BasicDownloadTestTraits::CreateLockInfo(Height height, state::LockStatus status) {
		return CreateDownloadEntry(
				GenerateRandomByteArray<Hash256>(),
				GenerateRandomByteArray<Key>(),
				GenerateRandomByteArray<Key>(),
				height,
				state::LockStatus::Used == status ? 0 : 2);
	}

	BasicDownloadTestTraits::ValueType BasicDownloadTestTraits::CreateLockInfo() {
		return CreateLockInfo(test::GenerateRandomValue<Height>());
	}

	void BasicDownloadTestTraits::SetKey(ValueType& downloadEntry, const KeyType& key) {
		downloadEntry.OperationToken = key;
	}

	void BasicDownloadTestTraits::AssertEqual(const ValueType& lhs, const ValueType& rhs) {
		AssertEqualDownloadData(lhs, rhs);
	}
}}
