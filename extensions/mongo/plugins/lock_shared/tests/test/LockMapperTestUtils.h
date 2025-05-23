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
#include "plugins/txes/lock_shared/src/state/LockInfo.h"
#include "catapult/utils/MemoryUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "tests/TestHarness.h"
#include <mongocxx/client.hpp>

namespace catapult { namespace test {

	/// Assert that non-inherited fields of a lock \a transaction match with corresponding fields of \a dbTransaction.
	template<typename TTransaction>
	void AssertEqualNonInheritedLockTransactionData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
		EXPECT_EQ(transaction.Duration, BlockDuration(test::GetUint64(dbTransaction, "duration")));
		EXPECT_EQ(transaction.Mosaic.MosaicId, UnresolvedMosaicId(test::GetUint64(dbTransaction, "mosaicId")));
		EXPECT_EQ(transaction.Mosaic.Amount, Amount(test::GetUint64(dbTransaction, "amount")));
	}

	/// Asserts that \a lockInfo with \a accountAddress is equal to \a dbLockInfo.
	inline void AssertEqualBaseLockInfoData(
			const state::LockInfo& lockInfo,
			const Address& accountAddress,
			const bsoncxx::document::view& dbLockInfo,
			bool multipleMosaics = false) {
		EXPECT_EQ(accountAddress, GetAddressValue(dbLockInfo, "accountAddress"));
		EXPECT_EQ(lockInfo.Account, GetKeyValue(dbLockInfo, "account"));
		if (multipleMosaics) {
			auto mosaicArray = dbLockInfo["mosaics"].get_array().value;
			ASSERT_EQ(lockInfo.Mosaics.size(), test::GetFieldCount(mosaicArray));
			for (const auto& dbMosaic : mosaicArray) {
				auto mosaicId = MosaicId(GetUint64(dbMosaic, "mosaicId"));
				EXPECT_EQ(lockInfo.Mosaics.at(mosaicId), Amount(GetUint64(dbMosaic, "amount")));
			}
		} else {
			EXPECT_EQ(lockInfo.Mosaics.begin()->first, MosaicId(GetUint64(dbLockInfo, "mosaicId")));
			EXPECT_EQ(lockInfo.Mosaics.begin()->second, Amount(GetUint64(dbLockInfo, "amount")));
		}
		EXPECT_EQ(lockInfo.Height, Height(GetUint64(dbLockInfo, "height")));
		EXPECT_EQ(lockInfo.Status, static_cast<state::LockStatus>(GetUint8(dbLockInfo, "status")));
	}
}}
