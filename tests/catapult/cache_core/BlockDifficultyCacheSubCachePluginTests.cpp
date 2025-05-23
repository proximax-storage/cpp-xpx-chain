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

#include "catapult/cache_core/BlockDifficultyCacheSubCachePlugin.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/mocks/MockMemoryStream.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS BlockDifficultyCacheSubCachePluginTests

	// region BlockDifficultyCacheSummaryCacheStorage - saveAll / saveSummary

	namespace {
		auto CreateConfigHolder() {
			test::MutableBlockchainConfiguration config;
			config.Network.MaxDifficultyBlocks = 111;
			return config::CreateMockConfigurationHolder(config.ToConst());
		}

		void RunSaveConsistencyTest(size_t numValues) {
			// Arrange:
			auto catapultCache = test::CoreSystemCacheFactory::Create();
			{
				auto cacheDelta = catapultCache.createDelta();
				auto& delta = cacheDelta.sub<BlockDifficultyCache>();
				for (auto i = 100u; i < 100 + numValues; ++i)
					delta.insert(Height(i), Timestamp(i), Difficulty(i));

				catapultCache.commit(Height(7));

				// Sanity:
				EXPECT_EQ(numValues, delta.size());
			}

			cache::BlockDifficultyCache cache(CreateConfigHolder());
			cache::BlockDifficultyCacheSummaryCacheStorage storage(cache);

			// Act: serialize via saveAll
			std::vector<uint8_t> bufferAll;
			mocks::MockMemoryStream streamAll(bufferAll);
			storage.saveAll(catapultCache.createView(), streamAll);

			// - serialize via saveSummary
			std::vector<uint8_t> bufferSummary;
			mocks::MockMemoryStream streamSummary(bufferSummary);
			storage.saveSummary(catapultCache.createDelta(), streamSummary);

			// Assert:
			EXPECT_EQ(bufferAll, bufferSummary);
		}
	}

	TEST(TEST_CLASS, SaveAllAndSaveSummaryWriteSameData_ZeroValues) {
		// Act + Assert:
		RunSaveConsistencyTest(0);
	}

	TEST(TEST_CLASS, SaveAllAndSaveSummaryWriteSameData_SingleValue) {
		// Act + Assert:
		RunSaveConsistencyTest(1);
	}

	TEST(TEST_CLASS, SaveAllAndSaveSummaryWriteSameData_MultipleValues) {
		// Act + Assert:
		RunSaveConsistencyTest(11);
	}

	// endregion

	// region BlockDifficultyCacheSubCachePlugin

	TEST(TEST_CLASS, CanCreateCacheStorageViaPlugin) {
		// Arrange:
		BlockDifficultyCacheSubCachePlugin plugin(CreateConfigHolder());

		// Act:
		auto pStorage = plugin.createStorage();

		// Assert:
		ASSERT_TRUE(!!pStorage);
		EXPECT_EQ("BlockDifficultyCache", pStorage->name());
	}

	// endregion
}}
