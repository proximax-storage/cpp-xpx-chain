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

#include "mongo/src/mappers/AccountStateMapper.h"
#include "catapult/model/Mosaic.h"
#include "catapult/state/AccountState.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/TestHarness.h"
#include <mongocxx/client.hpp>

namespace catapult { namespace mongo { namespace mappers {

#define TEST_CLASS AccountStateMapperTests

	// region ToDbModel

	namespace {
		void AssertEqualAccountStateMetadata(const bsoncxx::document::view& dbMetadata) {
			EXPECT_EQ(0u, test::GetFieldCount(dbMetadata));
		}

		auto CreateAccountState(Height publicKeyHeight, std::initializer_list<model::Mosaic> mosaics) {
			state::AccountState state(test::GenerateRandomAddress(), Height(123));
			if (Height(0) != publicKeyHeight) {
				state.PublicKeyHeight = publicKeyHeight;
				state.PublicKey = test::GenerateRandomData<Key_Size>();
			}

			for (const auto& mosaic : mosaics)
				state.Balances.credit(mosaic.MosaicId, mosaic.Amount, Height(123));

			return state;
		}

		void AssertCanMapAccountState(Height publicKeyHeight, std::initializer_list<model::Mosaic> mosaics) {
			// Arrange:
			auto state = CreateAccountState(publicKeyHeight, mosaics);

			// Act:
			auto dbAccount = ToDbModel(state);

			// Assert:
			auto view = dbAccount.view();
			EXPECT_EQ(2u, test::GetFieldCount(view));

			auto metaView = view["meta"].get_document().view();
			AssertEqualAccountStateMetadata(metaView);

			auto account = view["account"].get_document().view();
			EXPECT_EQ(5u, test::GetFieldCount(account));
			test::AssertEqualAccountState(state, account);
		}
	}

	TEST(TEST_CLASS, CanMapAccountStateWithNeitherPublicKeyNorMosaics) {
		// Assert:
		AssertCanMapAccountState(Height(0), {});
	}

	TEST(TEST_CLASS, CanMapAccountStateWithPublicKeyButWithoutMosaics) {
		// Assert:
		AssertCanMapAccountState(Height(456), {});
	}

	TEST(TEST_CLASS, CanMapAccountStateWithoutPublicKeyButWithSingleMosaic) {
		// Assert:
		AssertCanMapAccountState(Height(0), { { Xpx_Id, Amount(234) } });
	}

	TEST(TEST_CLASS, CanMapAccountStateWithoutPublicKeyButWithMultipleMosaics) {
		// Assert:
		AssertCanMapAccountState(Height(0), { { Xpx_Id, Amount(234) }, { MosaicId(1357), Amount(345) }, { MosaicId(31), Amount(45) } });
	}

	TEST(TEST_CLASS, CanMapAccountStateWithPublicKeyAndSingleMosaic) {
		// Assert:
		AssertCanMapAccountState(Height(456), { { Xpx_Id, Amount(234) } });
	}

	TEST(TEST_CLASS, CanMapAccountStateWithPublicKeyAndMultipleMosaics) {
		// Assert:
		AssertCanMapAccountState(Height(456), { { Xpx_Id, Amount(234) }, { MosaicId(1357), Amount(345) }, { MosaicId(31), Amount(45) } });
	}

	// endregion

	// region ToAccountState

	namespace {
		void AssertCanMapDbAccountState(Height publicKeyHeight, std::initializer_list<model::Mosaic> mosaics) {
			// Arrange:
			auto state = CreateAccountState(publicKeyHeight, mosaics);
			auto dbAccount = ToDbModel(state);

			// Act:
			state::AccountState newAccountState(Address(), Height(0));
			ToAccountState(dbAccount, [&newAccountState](const auto& address, auto height) -> state::AccountState& {
				newAccountState.Address = address;
				newAccountState.AddressHeight = height;
				return newAccountState;
			});

			// Assert:
			auto accountData = dbAccount.view()["account"].get_document();
			test::AssertEqualAccountState(newAccountState, accountData.view());
		}
	}

	TEST(TEST_CLASS, CanMapDbAccountStateWithNeitherPublicKeyNorMosaics) {
		// Assert:
		AssertCanMapDbAccountState(Height(0), {});
	}

	TEST(TEST_CLASS, CanMapDbAccountStateWithPublicKeyButWithoutMosaics) {
		// Assert:
		AssertCanMapDbAccountState(Height(456), {});
	}

	TEST(TEST_CLASS, CanMapDbAccountStateWithoutPublicKeyButWithSingleMosaic) {
		// Assert:
		AssertCanMapDbAccountState(Height(0), { { Xpx_Id, Amount(234) } });
	}

	TEST(TEST_CLASS, CanMapDbAccountStateWithoutPublicKeyButWithMultipleMosaics) {
		// Assert:
		AssertCanMapDbAccountState(Height(0), { { Xpx_Id, Amount(234) }, { MosaicId(1357), Amount(345) }, { MosaicId(31), Amount(45) } });
	}

	TEST(TEST_CLASS, CanMapDbAccountStateWithPublicKeyAndSingleMosaic) {
		// Assert:
		AssertCanMapDbAccountState(Height(456), { { Xpx_Id, Amount(234) } });
	}

	TEST(TEST_CLASS, CanMapDbAccountStateWithPublicKeyAndMultipleMosaics) {
		// Assert:
		AssertCanMapDbAccountState(
				Height(456),
				{ { Xpx_Id, Amount(234) }, { MosaicId(1357), Amount(345) }, { MosaicId(31), Amount(45) } });
	}

	// endregion
}}}
