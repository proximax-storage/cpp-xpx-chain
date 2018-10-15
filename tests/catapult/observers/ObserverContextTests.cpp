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

#include "catapult/observers/ObserverContext.h"
#include "catapult/cache/CatapultCache.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS ObserverContextTests

	namespace {
		constexpr auto Default_Height = Height(123);

		template<typename TAction>
		void RunTestWithStateAndCache(TAction action) {
			// Arrange:
			cache::CatapultCache cache({});
			auto cacheDelta = cache.createDelta();

			// Act:
			action(cacheDelta);
		}
	}

	TEST(TEST_CLASS, CanCreateObserverState) {
		// Act:
		RunTestWithStateAndCache([](auto& cacheDelta) {
			ObserverState observerState(cacheDelta);

			// Assert:
			EXPECT_EQ(&cacheDelta, &observerState.Cache);
		});
	}

	TEST(TEST_CLASS, CanCreateCommitObserverContextAroundObserverState) {
		// Act:
		RunTestWithStateAndCache([](auto& cacheDelta) {
			ObserverContext context(ObserverState(cacheDelta), Default_Height, NotifyMode::Commit);

			// Assert:
			EXPECT_EQ(&cacheDelta, &context.Cache);
			EXPECT_EQ(Default_Height, context.Height);
			EXPECT_EQ(NotifyMode::Commit, context.Mode);
		});
	}

	TEST(TEST_CLASS, CanCreateCommitObserverContextAroundCacheAndState) {
		// Act:
		RunTestWithStateAndCache([](auto& cacheDelta) {
			ObserverContext context(cacheDelta, Default_Height, NotifyMode::Commit);

			// Assert:
			EXPECT_EQ(&cacheDelta, &context.Cache);
			EXPECT_EQ(Default_Height, context.Height);
			EXPECT_EQ(NotifyMode::Commit, context.Mode);
		});
	}

	TEST(TEST_CLASS, CanCreateRollbackObserverContextAroundObserverState) {
		// Act:
		RunTestWithStateAndCache([](auto& cacheDelta) {
			ObserverContext context(ObserverState(cacheDelta), Default_Height, NotifyMode::Rollback);

			// Assert:
			EXPECT_EQ(&cacheDelta, &context.Cache);
			EXPECT_EQ(Default_Height, context.Height);
			EXPECT_EQ(NotifyMode::Rollback, context.Mode);
		});
	}

	TEST(TEST_CLASS, CanCreateRollbackObserverContextAroundCacheAndState) {
		// Act:
		RunTestWithStateAndCache([](auto& cacheDelta) {
			ObserverContext context(cacheDelta, Default_Height, NotifyMode::Rollback);

			// Assert:
			EXPECT_EQ(&cacheDelta, &context.Cache);
			EXPECT_EQ(Default_Height, context.Height);
			EXPECT_EQ(NotifyMode::Rollback, context.Mode);
		});
	}
}}
