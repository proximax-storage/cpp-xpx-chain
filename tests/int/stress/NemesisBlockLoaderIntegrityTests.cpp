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

#include "catapult/extensions/LocalNodeChainScore.h"
#include "catapult/extensions/NemesisBlockLoader.h"
#include "tests/test/local/LocalNodeTestState.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/nemesis/NemesisCompatibleConfiguration.h"
#include "tests/test/nemesis/NemesisTestUtils.h"
#include "tests/test/nodeps/Filesystem.h"
#include "tests/test/nodeps/MijinConstants.h"

namespace catapult { namespace extensions {

#define TEST_CLASS NemesisBlockLoaderIntegrityTests

	namespace {
		// region test utils

		// Num_Nemesis_Transactions - nemesis block has:
		// 1) Num_Nemesis_Namespaces register namespace transactions
		// 2) for each mosaic - (a) mosaic definition transaction, (b) mosaic supply change transaction, (c) mosaic alias transaction
		// 3) Num_Nemesis_Accounts transfer transactions

		constexpr auto Num_Nemesis_Accounts = CountOf(test::Mijin_Test_Private_Keys);
		constexpr auto Num_Nemesis_Namespaces = 6;
		constexpr auto Num_Nemesis_Mosaics = 5;
		constexpr auto Num_Nemesis_Config = 1;
		constexpr auto Num_Nemesis_Upgrade = 1;
		constexpr auto Num_Nemesis_AddHarvester = 22;
		constexpr auto Num_Nemesis_Transactions =
			Num_Nemesis_Namespaces +
			3 * Num_Nemesis_Mosaics +
			Num_Nemesis_Accounts +
			Num_Nemesis_Config +
			Num_Nemesis_Upgrade +
			Num_Nemesis_AddHarvester;

		template<typename TAction>
		void RunNemesisBlockTest(TAction action) {
			// Arrange:
			test::TempDirectoryGuard tempDir;

			auto config = test::CreatePrototypicalBlockchainConfiguration();
			const_cast<config::UserConfiguration&>(config.User).DataDirectory = tempDir.name();
			test::AddNemesisPluginExtensions(const_cast<model::NetworkConfiguration&>(config.Network));

			auto pPluginManager = test::CreatePluginManagerWithRealPlugins(config);
			test::LocalNodeTestState localNodeState(config, pPluginManager->createCache());

			{
				auto cacheDelta = localNodeState.ref().Cache.createDelta();

				// Act:
				NemesisBlockLoader loader(cacheDelta, *pPluginManager, pPluginManager->createObserver());
				loader.executeAndCommit(localNodeState.ref(), StateHashVerification::Enabled);
			}

			// Assert:
			action(localNodeState.ref());
		}

		// endregion
	}

	// region tests

	TEST(TEST_CLASS, ProperAccountStateAfterLoadingNemesisBlock) {
		// Act:
		RunNemesisBlockTest([](const auto& stateRef) {
			// Assert:
			const auto& cacheView = stateRef.Cache.createView();
			EXPECT_EQ(Height(1), cacheView.height());
			test::AssertNemesisAccountState(cacheView);
		});
	}

	TEST(TEST_CLASS, ProperMosaicStateAfterLoadingNemesisBlock) {
		// Act:
		RunNemesisBlockTest([](const auto& stateRef) {
			// Assert:
			const auto& cacheView = stateRef.Cache.createView();
			test::AssertNemesisNamespaceState(cacheView);
			test::AssertNemesisMosaicState(cacheView);
		});
	}

	TEST(TEST_CLASS, ProperChainScoreAfterLoadingNemesisBlock) {
		// Act:
		RunNemesisBlockTest([](const auto& stateRef) {
			// Assert: NemesisBlockLoader doesn't modify score
			EXPECT_EQ(model::ChainScore(0), stateRef.Score.get());
		});
	}

	TEST(TEST_CLASS, ProperRecalculationHeightAfterLoadingNemesisBlock) {
		// Act:
		RunNemesisBlockTest([](const auto& stateRef) {
			// Assert:
			EXPECT_EQ(model::ImportanceHeight(), stateRef.State.LastRecalculationHeight);
		});
	}

	TEST(TEST_CLASS, ProperNumTransactionsAfterLoadingNemesisBlock) {
		// Act:
		RunNemesisBlockTest([](const auto& stateRef) {
			// Assert:
			EXPECT_EQ(Num_Nemesis_Transactions, stateRef.State.NumTotalTransactions.load());
		});
	}

	// endregion
}}
