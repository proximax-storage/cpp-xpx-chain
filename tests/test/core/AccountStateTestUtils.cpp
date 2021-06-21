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

#include "AccountStateTestUtils.h"
#include "AddressTestUtils.h"
#include "tests/test/nodeps/TestConstants.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	void RandomFillAccountData(uint64_t seed, state::AccountState& state, size_t numMosaics, size_t numSnapshots) {
		state.Balances.track(test::Default_Harvesting_Mosaic_Id);
		for (auto i = 0u; i < numMosaics; ++i) {
			state.Balances.credit(MosaicId(10 + i), Amount(seed * 1000 + i + 1));
		}

		for (auto i = 0u; i < numSnapshots; ++i) {
			state.Balances.addSnapshot(model::BalanceSnapshot{Amount(seed * 1000 + i + 1), state.AddressHeight + Height(i + 1)});
		}
	}

	void AssertEqual(const state::AccountState& expected, const state::AccountState& actual, const std::string& message) {
		// Assert:
		EXPECT_EQ(expected.Address, actual.Address) << message;
		EXPECT_EQ(expected.AddressHeight, actual.AddressHeight) << message;
		EXPECT_EQ(expected.PublicKey, actual.PublicKey) << message;
		EXPECT_EQ(expected.PublicKeyHeight, actual.PublicKeyHeight) << message;

		EXPECT_EQ(expected.AccountType, actual.AccountType) << message;
		EXPECT_EQ(expected.SupplementalPublicKeys.linked().get(), actual.SupplementalPublicKeys.linked().get()) << message;
		EXPECT_EQ(expected.SupplementalPublicKeys.node().get(), actual.SupplementalPublicKeys.node().get()) << message;
		EXPECT_EQ(expected.Balances.size(), actual.Balances.size()) << message;
		EXPECT_EQ(expected.Balances.size(), actual.Balances.size());
		for (const auto& pair : expected.Balances)
			EXPECT_EQ(pair.second, actual.Balances.get(pair.first)) << message << ": for mosaic " << pair.first;

		const auto& expectedSnapshots = expected.Balances.snapshots();
		const auto& actualSnapshots = actual.Balances.snapshots();
		for (auto expectedIt = expectedSnapshots.begin(), actualIt = actualSnapshots.begin();
			expectedIt != expectedSnapshots.end();
			++expectedIt, ++actualIt) {
			EXPECT_EQ(expectedIt->BalanceHeight, actualIt->BalanceHeight);
			EXPECT_EQ(expectedIt->Amount, actualIt->Amount);
		}
	}

	std::shared_ptr<state::AccountState> CreateAccountStateWithoutPublicKey(uint64_t height) {
		auto address = test::GenerateRandomAddress();
		auto pState = std::make_shared<state::AccountState>(address, Height(height));
		pState->Balances.track(test::Default_Harvesting_Mosaic_Id);
		pState->Balances.credit(test::Default_Harvesting_Mosaic_Id, Amount(1));
		return pState;
	}

	AccountStates CreateAccountStates(size_t count) {
		AccountStates accountStates;
		for (auto i = 1u; i <= count; ++i) {
			accountStates.push_back(test::CreateAccountStateWithoutPublicKey(1));
			accountStates.back()->PublicKey = { { static_cast<uint8_t>(i) } };
		}

		return accountStates;
	}
}}
