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

#include "catapult/state/AccountStateAdapter.h"
#include "tests/test/core/AccountStateTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS AccountStateAdapterTests

	// region AccountState -> AccountInfo

	namespace {
		AccountState CreateAccountStateWithZeroMosaics() {
			AccountState accountState(test::GenerateRandomData<Address_Decoded_Size>(), Height(123));
			accountState.PublicKey = test::GenerateRandomData<Key_Size>();
			accountState.PublicKeyHeight = Height(456);

			return accountState;
		}

		void AssertNonMosaicPart(const model::AccountInfo& info, const AccountState& accountState, uint32_t expectedMosaicsCount, uint32_t expectedSnapshotCount) {
			// Assert:
			EXPECT_EQ(sizeof(model::AccountInfo) + expectedMosaicsCount * sizeof(model::Mosaic) + expectedSnapshotCount * sizeof(model::BalanceSnapshot), info.Size);
			EXPECT_EQ(accountState.Address, info.Address);
			EXPECT_EQ(accountState.AddressHeight, info.AddressHeight);
			EXPECT_EQ(accountState.PublicKey, info.PublicKey);
			EXPECT_EQ(accountState.PublicKeyHeight, info.PublicKeyHeight);
			EXPECT_EQ(expectedMosaicsCount, info.MosaicsCount);
		}

		void AssertMosaic(const model::Mosaic& mosaic, MosaicId expectedId, Amount expectedAmount) {
			// Assert:
			EXPECT_EQ(expectedId, mosaic.MosaicId);
			EXPECT_EQ(expectedAmount, mosaic.Amount);
		}

		void AssertSnapshot(const model::BalanceSnapshot& snapshot, Amount expectedAmount, Height height) {
			// Assert:
			EXPECT_EQ(height, snapshot.BalanceHeight);
			EXPECT_EQ(expectedAmount, snapshot.Amount);
		}

		void AssertToAccountInfoInitializesAllAccountInfoFieldsZeroMosaics() {
			// Arrange:
			auto accountState = CreateAccountStateWithZeroMosaics();

			// Act:
			auto pAccountInfo = ToAccountInfo(accountState);

			// Assert:
			AssertNonMosaicPart(*pAccountInfo, accountState, 0, 0);
			EXPECT_FALSE(!!pAccountInfo->MosaicsPtr());
		}
	}

	TEST(TEST_CLASS, ToAccountInfoInitializesAllAccountInfoFields_ZeroMosaics) {
		// Assert:
		AssertToAccountInfoInitializesAllAccountInfoFieldsZeroMosaics();
	}

	TEST(TEST_CLASS, ToAccountInfoInitializesAllAccountInfoFields_SingleMosaic) {
		// Arrange:
		auto accountState = CreateAccountStateWithZeroMosaics();
		accountState.Balances.credit(Xpx_Id, Amount(13579), Height(1));

		// Act:
		auto pAccountInfo = ToAccountInfo(accountState);

		// Assert:
		AssertNonMosaicPart(*pAccountInfo, accountState, 1, 1);

		auto pMosaic = pAccountInfo->MosaicsPtr();
		auto pSnapshot = pAccountInfo->BalanceSnapshotPtr();
		ASSERT_TRUE(!!pMosaic);
		AssertMosaic(*pMosaic, Xpx_Id, Amount(13579));
		ASSERT_TRUE(!!pSnapshot);
		AssertSnapshot(*pSnapshot, Amount(13579), Height(1));
	}

	TEST(TEST_CLASS, ToAccountInfoInitializesAllAccountInfoFields_MultipleMosaics) {
		// Arrange:
		auto accountState = CreateAccountStateWithZeroMosaics();
		accountState.Balances.credit(MosaicId(123), Amount(111), Height(1));
		accountState.Balances.credit(Xpx_Id, Amount(13579), Height(1));
		accountState.Balances.credit(MosaicId(987), Amount(222), Height(1));

		// Act:
		auto pAccountInfo = ToAccountInfo(accountState);

		// Assert:
		AssertNonMosaicPart(*pAccountInfo, accountState, 3, 1);

		auto pMosaic = pAccountInfo->MosaicsPtr();
		ASSERT_TRUE(!!pMosaic);

		std::map<MosaicId, model::Mosaic> mosaics;
		for (auto i = 0u; i < 3; ++i) {
			mosaics.emplace(pMosaic->MosaicId, *pMosaic);
			++pMosaic;
		}

		AssertMosaic(mosaics[MosaicId(123)], MosaicId(123), Amount(111));
		AssertMosaic(mosaics[Xpx_Id], Xpx_Id, Amount(13579));
		AssertMosaic(mosaics[MosaicId(987)], MosaicId(987), Amount(222));

		auto pSnapshot = pAccountInfo->BalanceSnapshotPtr();
		ASSERT_TRUE(!!pSnapshot);
		AssertSnapshot(*pSnapshot, Amount(13579), Height(1));
	}

	// endregion

	// region AccountState <- AccountInfo

	namespace {
		void AssertCanCreateAccountStateFromAccountInfo(const std::vector<model::Mosaic>& mosaics) {
			// Arrange:
			auto originalAccountState = CreateAccountStateWithZeroMosaics();
			for (const auto& mosaic : mosaics)
				originalAccountState.Balances.credit(mosaic.MosaicId, mosaic.Amount, Height(1));

			auto pAccountInfo = ToAccountInfo(originalAccountState);

			// Act:
			auto accountState = ToAccountState(*pAccountInfo);

			// Assert:
			EXPECT_EQ(mosaics.size(), accountState.Balances.size());
			test::AssertEqual(originalAccountState, accountState);
		}
	}

	TEST(TEST_CLASS, CanCreateAccountStateFromAccountInfo_ZeroMosaics) {
		// Assert:
		AssertCanCreateAccountStateFromAccountInfo({});
	}

	TEST(TEST_CLASS, CanCreateAccountStateFromAccountInfo_SingleMosaic) {
		// Assert:
		AssertCanCreateAccountStateFromAccountInfo({ { Xpx_Id, Amount(13579) } });
	}

	TEST(TEST_CLASS, CanCreateAccountStateFromAccountInfo_MultipleMosaics) {
		// Assert:
		AssertCanCreateAccountStateFromAccountInfo({
			{ MosaicId(123), Amount(111) },
			{ Xpx_Id, Amount(13579) },
			{ MosaicId(987), Amount(222) }
		});
	}

	// endregion
}}
