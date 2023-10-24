/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/utils/NetworkTime.h"
#include "tests/test/DbrbTestUtils.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"

namespace catapult { namespace cache {

#define TEST_CLASS DbrbViewCacheTests

	namespace {
		struct DbrbViewCacheMixinTraits {
			class CacheType : public DbrbViewCache {
			public:
				CacheType(std::shared_ptr<DbrbViewFetcherImpl> pDbrbViewFetcher) : DbrbViewCache(CacheConfiguration(), config::CreateMockConfigurationHolder(), std::move(pDbrbViewFetcher))
				{}
			};
		};

		struct ProcessIdExpirationTime {
			dbrb::ProcessId ProcessId;
			Timestamp ExpirationTime;
		};
	}

	// region custom tests

	TEST(TEST_CLASS, DbrbViewIsSelectedCorrectly) {
		// Arrange:
		auto pDbrbViewFetcher = std::make_shared<DbrbViewFetcherImpl>();
		DbrbViewCacheMixinTraits::CacheType cache(pDbrbViewFetcher);
		auto now = utils::NetworkTime();
		std::vector<ProcessIdExpirationTime> processes {
			{ dbrb::ProcessId({  0 }), now + Timestamp(5000) },
			{ dbrb::ProcessId({  1 }), now + Timestamp(4000) },
			{ dbrb::ProcessId({  2 }), now + Timestamp(3000) },
			{ dbrb::ProcessId({  3 }), now + Timestamp(2000) },
			{ dbrb::ProcessId({  4 }), now + Timestamp(1000) },
			{ dbrb::ProcessId({  5 }), now },
			{ dbrb::ProcessId({  6 }), now - Timestamp(1000) },
			{ dbrb::ProcessId({  7 }), now - Timestamp(2000) },
			{ dbrb::ProcessId({  8 }), now - Timestamp(3000) },
			{ dbrb::ProcessId({  9 }), now - Timestamp(4000) },
			{ dbrb::ProcessId({ 10 }), now - Timestamp(5000) },
		};

		std::vector<ProcessIdExpirationTime> additionalProcesses {
			{ dbrb::ProcessId({  11 }), now + Timestamp(4000) },
			{ dbrb::ProcessId({  12 }), now + Timestamp(3000) },
			{ dbrb::ProcessId({  13 }), now + Timestamp(2000) },
			{ dbrb::ProcessId({  14 }), now + Timestamp(1000) },
		};

		// Initial state assert:
		{
			auto cacheView = cache.createView(Height(0));
			EXPECT_EQ(0, cacheView->size());
			EXPECT_EQ(dbrb::ViewData{}, pDbrbViewFetcher->getView(utils::NetworkTime()));
		}

		// Inserting processes.
		{
			// Act:
			auto cacheDelta = cache.createDelta(Height(0));
			for (const auto& [processId, expirationTime] : processes)
				cacheDelta->insert(test::CreateDbrbProcessEntry(processId, expirationTime));
			cache.commit();

			// Assert:
			auto cacheView = cache.createView(Height(0));
			EXPECT_EQ(processes.size(), cacheView->size());

			dbrb::ViewData expectedView{
				processes[0].ProcessId,
				processes[1].ProcessId,
				processes[2].ProcessId,
				processes[3].ProcessId,
				processes[4].ProcessId,
			};
			auto actualView = pDbrbViewFetcher->getView(now);
			EXPECT_EQ(expectedView, actualView);

			expectedView.emplace(processes[5].ProcessId);
			expectedView.emplace(processes[6].ProcessId);
			actualView = pDbrbViewFetcher->getView(now - Timestamp(2000));
			EXPECT_EQ(expectedView, actualView);
		}

		// Removing processes.
		{
			// Act:

			auto cacheDelta = cache.createDelta(Height(1));
			for (auto i = 0; i < processes.size() / 2 + 1; ++i)
				cacheDelta->remove(processes[2 * i].ProcessId);
			for (const auto& [processId, expirationTime] : additionalProcesses)
				cacheDelta->insert(test::CreateDbrbProcessEntry(processId, expirationTime));
			cache.commit();

			// Assert:
			auto cacheView = cache.createView(Height(1));
			EXPECT_EQ(5u + additionalProcesses.size(), cacheView->size());

			dbrb::ViewData expectedView{
				processes[1].ProcessId,
				processes[3].ProcessId,
				additionalProcesses[0].ProcessId,
				additionalProcesses[1].ProcessId,
				additionalProcesses[2].ProcessId,
				additionalProcesses[3].ProcessId,
			};
			auto actualView = pDbrbViewFetcher->getView(now);
			EXPECT_EQ(expectedView, actualView);
		}
	}

	// endregion

}}

