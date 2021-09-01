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
		constexpr MosaicId Test_Mosaic = MosaicId(12345);

		void AssertEqualAccountStateMetadata(const bsoncxx::document::view& dbMetadata) {
			EXPECT_EQ(0u, test::GetFieldCount(dbMetadata));
		}

		template<uint32_t TVersion>
		auto CreateAccountState(Height publicKeyHeight, std::initializer_list<model::Mosaic> mosaics, std::initializer_list<model::BalanceSnapshot> snapshots) {
			state::AccountState state(test::GenerateRandomAddress(), Height(123), TVersion);
			if (Height(0) != publicKeyHeight) {
				state.PublicKeyHeight = publicKeyHeight;
				test::FillWithRandomData(state.PublicKey);
			}

			state.AccountType = static_cast<state::AccountType>(34);
			Key temp;
			test::FillWithRandomData(temp);
			state.SupplementalPublicKeys.linked().unset();
			state.SupplementalPublicKeys.linked().set(std::move(temp));
			test::FillWithRandomData(temp);
			state.SupplementalPublicKeys.vrf().unset();
			state.SupplementalPublicKeys.vrf().set(std::move(temp));
			test::FillWithRandomData(temp);
			state.SupplementalPublicKeys.node().unset();
			state.SupplementalPublicKeys.node().set(std::move(temp));
			state.Balances.track(Test_Mosaic);
			for (const auto& mosaic : mosaics)
				state.Balances.credit(mosaic.MosaicId, mosaic.Amount);

			for (const auto& snapshot : snapshots)
				state.Balances.addSnapshot(snapshot);

			return state;
		}

		template<uint32_t TVersion>
		void AssertCanMapAccountState(Height publicKeyHeight, std::initializer_list<model::Mosaic> mosaics, std::initializer_list<model::BalanceSnapshot> snapshots) {
			// Arrange:
			auto state = CreateAccountState<TVersion>(publicKeyHeight, mosaics, snapshots);

			// Act:
			auto dbAccount = ToDbModel(state);

			// Assert:
			auto view = dbAccount.view();
			EXPECT_EQ(2u, test::GetFieldCount(view));

			auto metaView = view["meta"].get_document().view();
			AssertEqualAccountStateMetadata(metaView);

			auto account = view["account"].get_document().view();
			EXPECT_EQ(9u, test::GetFieldCount(account)); //Keep in mind backwards compatible check for this, handle later!!!!!
			test::AssertEqualAccountState(state, account);
		}
	}

	TEST(TEST_CLASS, CanMapAccountStateWithNeitherPublicKeyNotMosaicsNotSnapshots) {
		// Assert:
		AssertCanMapAccountState<1>(Height(0), {}, {});
		AssertCanMapAccountState<2>(Height(0), {}, {});
	}

	TEST(TEST_CLASS, CanMapAccountStateWithPublicKeyButWithoutMosaicsAnsSNapshots) {
		// Assert:
		AssertCanMapAccountState<1>(Height(456), {}, {});
		AssertCanMapAccountState<2>(Height(456), {}, {});
	}

	TEST(TEST_CLASS, CanMapAccountStateWithoutPublicKeyButWithSingleMosaicAndSnapshot) {
		// Assert:
		AssertCanMapAccountState<1>(Height(0), { { Test_Mosaic, Amount(234) } }, { { Amount(234), Height(123) } });
		AssertCanMapAccountState<2>(Height(0), { { Test_Mosaic, Amount(234) } }, { { Amount(234), Height(123) } });
	}

	TEST(TEST_CLASS, CanMapAccountStateWithoutPublicKeyButWithMultipleMosaicsAndSingleSnapshot) {
		// Assert:
		AssertCanMapAccountState<1>(
			Height(0),
			{
				{ Test_Mosaic, Amount(234) },
				{ MosaicId(1357), Amount(345) },
				{ MosaicId(31), Amount(45) }
			},
			{
				{ Amount(234), Height(123) }
			}
		);
		AssertCanMapAccountState<2>(
				Height(0),
				{
						{ Test_Mosaic, Amount(234) },
						{ MosaicId(1357), Amount(345) },
						{ MosaicId(31), Amount(45) }
				},
				{
						{ Amount(234), Height(123) }
				}
		);
	}

	TEST(TEST_CLASS, CanMapAccountStateWithPublicKeyAndSingleMosaicAndSnapshot) {
		// Assert:
		AssertCanMapAccountState<1>(Height(456), { { Test_Mosaic, Amount(234) } }, { { Amount(234), Height(456) } });
		AssertCanMapAccountState<2>(Height(456), { { Test_Mosaic, Amount(234) } }, { { Amount(234), Height(456) } });
	}

	TEST(TEST_CLASS, CanMapAccountStateWithPublicKeyAndMultipleMosaicsAndSnapshots) {
		// Assert:
		AssertCanMapAccountState<1>(
			Height(456),
			{
				{ Test_Mosaic, Amount(234) },
				{ MosaicId(1357), Amount(345) },
				{ MosaicId(31), Amount(45) }
			},
			{
				{ Amount(234), Height(456) },
				{ Amount(235), Height(457) },
				{ Amount(236), Height(458) }
			}
		);
		AssertCanMapAccountState<2>(
				Height(456),
				{
						{ Test_Mosaic, Amount(234) },
						{ MosaicId(1357), Amount(345) },
						{ MosaicId(31), Amount(45) }
				},
				{
						{ Amount(234), Height(456) },
						{ Amount(235), Height(457) },
						{ Amount(236), Height(458) }
				}
		);
	}

	// endregion

	// region ToAccountState

	namespace {
		template<uint32_t TVersion>
		void AssertCanMapDbAccountState(Height publicKeyHeight, std::initializer_list<model::Mosaic> mosaics, std::initializer_list<model::BalanceSnapshot> snapshots) {
			// Arrange:
			auto state = CreateAccountState<TVersion>(publicKeyHeight, mosaics, snapshots);
			auto dbAccount = ToDbModel(state);

			// Act:
			std::shared_ptr<state::AccountState> statePtr;
			ToAccountState(dbAccount, [&statePtr](const auto& address, auto height, auto version) -> std::shared_ptr<state::AccountState>  {

			  	statePtr = std::make_shared<state::AccountState>(Address(), Height(0), version);
				statePtr->Address = address;
				statePtr->AddressHeight = height;
				return statePtr;
			});

			// Assert:
			auto accountData = dbAccount.view()["account"].get_document();
			test::AssertEqualAccountState(*statePtr, accountData.view());
		}
	}

	TEST(TEST_CLASS, CanMapDbAccountStateWithNeitherPublicKeyNotMosaicsAndSnapshot) {
		// Assert:
		AssertCanMapDbAccountState<1>(Height(0), {}, {});
		AssertCanMapDbAccountState<2>(Height(0), {}, {});
	}

	TEST(TEST_CLASS, CanMapDbAccountStateWithPublicKeyButWithoutMosaicsAndSnapshot) {
		// Assert:
		AssertCanMapDbAccountState<1>(Height(456), {}, {});
		AssertCanMapDbAccountState<2>(Height(456), {}, {});
	}

	TEST(TEST_CLASS, CanMapDbAccountStateWithoutPublicKeyButWithSingleMosaicAnsWithoutSnapshot) {
		// Assert:
		AssertCanMapDbAccountState<1>(Height(0), { { Test_Mosaic, Amount(234) } }, {});
		AssertCanMapDbAccountState<2>(Height(0), { { Test_Mosaic, Amount(234) } }, {});
	}

	TEST(TEST_CLASS, CanMapDbAccountStateWithoutPublicKeyButWithWithoutMosaicAnsSingleSnapshot) {
		// Assert:
		AssertCanMapDbAccountState<1>(Height(0), {}, { { Amount(234), Height(456) } });
		AssertCanMapDbAccountState<2>(Height(0), {}, { { Amount(234), Height(456) } });
	}

	TEST(TEST_CLASS, CanMapDbAccountStateWithoutPublicKeyButWithSingleMosaicAnsSnapshot) {
		// Assert:
		AssertCanMapDbAccountState<1>(Height(0), { { Test_Mosaic, Amount(234) } }, { { Amount(234), Height(456) } });
		AssertCanMapDbAccountState<2>(Height(0), { { Test_Mosaic, Amount(234) } }, { { Amount(234), Height(456) } });
	}

	TEST(TEST_CLASS, CanMapDbAccountStateWithoutPublicKeyButWithMultipleMosaicsAndSingleSnapshot) {
		// Assert:
		AssertCanMapDbAccountState<1>(
			Height(0),
			{
				{ Test_Mosaic, Amount(234) },
				{ MosaicId(1357), Amount(345) },
				{ MosaicId(31), Amount(45) }
			},
			{
				{ Amount(234), Height(456) }
			}
		);
		AssertCanMapDbAccountState<2>(
				Height(0),
				{
						{ Test_Mosaic, Amount(234) },
						{ MosaicId(1357), Amount(345) },
						{ MosaicId(31), Amount(45) }
				},
				{
						{ Amount(234), Height(456) }
				}
		);
	}

	TEST(TEST_CLASS, CanMapDbAccountStateWithPublicKeyAndSingleMosaicAndWithoutSnapshot) {
		// Assert:
		AssertCanMapDbAccountState<1>(Height(456), { { Test_Mosaic, Amount(234) } }, {});
		AssertCanMapDbAccountState<2>(Height(456), { { Test_Mosaic, Amount(234) } }, {});
	}

	TEST(TEST_CLASS, CanMapDbAccountStateWithPublicKeyAndWithoutMosaicAndSingleSnapshot) {
		// Assert:
		AssertCanMapDbAccountState<1>(Height(456), {}, { { Amount(234), Height(456) } });
		AssertCanMapDbAccountState<2>(Height(456), {}, { { Amount(234), Height(456) } });
	}

	TEST(TEST_CLASS, CanMapDbAccountStateWithPublicKeyAndSingleMosaicAndSnapshot) {
		// Assert:
		AssertCanMapDbAccountState<1>(Height(456), { { Test_Mosaic, Amount(234) } }, { { Amount(234), Height(456) } });
		AssertCanMapDbAccountState<2>(Height(456), { { Test_Mosaic, Amount(234) } }, { { Amount(234), Height(456) } });
	}

	TEST(TEST_CLASS, CanMapDbAccountStateWithPublicKeyAndMultipleMosaicsAndSnapshots) {
		// Assert:
		AssertCanMapDbAccountState<1>(
			Height(456),
			{
				{ Test_Mosaic, Amount(234) },
				{ MosaicId(1357), Amount(345) },
				{ MosaicId(31), Amount(45) }
			},
			{
				{ Amount(234), Height(456) } ,
				{ Amount(235), Height(457) } ,
				{ Amount(236), Height(458) }
			}
		);
		AssertCanMapDbAccountState<2>(
				Height(456),
				{
						{ Test_Mosaic, Amount(234) },
						{ MosaicId(1357), Amount(345) },
						{ MosaicId(31), Amount(45) }
				},
				{
						{ Amount(234), Height(456) } ,
						{ Amount(235), Height(457) } ,
						{ Amount(236), Height(458) }
				}
		);
	}

	// endregion
}}}
