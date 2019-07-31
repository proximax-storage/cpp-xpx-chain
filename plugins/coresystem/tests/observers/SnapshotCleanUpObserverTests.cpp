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

#include "src/observers/Observers.h"
#include "tests/test/cache/BalanceTransferTestUtils.h"
#include "tests/test/core/mocks/MockLocalNodeConfigurationHolder.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "tests/test/plugins/AccountObserverTestContext.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS SnapshotCleanUpObserverTests

		DEFINE_COMMON_OBSERVER_TESTS(SnapshotCleanUp, config::CreateMockConfigurationHolder())

		const uint64_t Effective_Balance_Range = 10;
		const uint64_t Max_Rollback_Blocks = 1;

		namespace {
			void AssertCleanUpSnapshots(Height contextHeight, uint64_t expectedSizeOfSnapshots,
										observers::NotifyMode mode = observers::NotifyMode::Commit) {
				// Arrange:
				auto config = model::BlockChainConfiguration::Uninitialized();
				config.ImportanceGrouping = Effective_Balance_Range;
				config.MaxRollbackBlocks = Max_Rollback_Blocks;
				auto pConfigHolder = config::CreateMockConfigurationHolder(config);

				test::AccountObserverTestContext context(mode, contextHeight, config);

				auto pObserver = CreateSnapshotCleanUpObserver(pConfigHolder);

				auto signer = test::GenerateRandomByteArray<Key>();
				for (auto i = 0u; i < Effective_Balance_Range + Max_Rollback_Blocks + 1; ++i) {
					context.setAccountBalance<Key>(signer, Amount(1), Height(i + 1));
				}
				context.commitCacheChanges();

				auto account = context.find(signer);
				EXPECT_EQ(Effective_Balance_Range + Max_Rollback_Blocks + 1, account->Balances.snapshots().size());

				auto notification = test::CreateBlockNotification(signer);
				// Act:
				test::ObserveNotification(*pObserver, notification, context);

				account = context.find(signer);
				EXPECT_EQ(expectedSizeOfSnapshots, account->Balances.snapshots().size());
			}
		}

		TEST(TEST_CLASS, CleanUpSnapshotsOnDifferentHeights) {
			for (auto i = 0u; i <= Effective_Balance_Range + Max_Rollback_Blocks; ++i) {
				AssertCleanUpSnapshots(Height(Effective_Balance_Range + Max_Rollback_Blocks + 1 + i), Effective_Balance_Range + Max_Rollback_Blocks - i);
			}

			AssertCleanUpSnapshots(Height(1000), 0);
		}

		TEST(TEST_CLASS, CleanUpSnapshotsDuringRollbackMode) {
			AssertCleanUpSnapshots(Height(1000), Effective_Balance_Range + Max_Rollback_Blocks + 1, observers::NotifyMode::Rollback);
		}
	}
}
