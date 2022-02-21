/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#include "tests/test/LockFundTestUtils.h"
#include "src/cache/LockFundCacheStorage.h"


namespace catapult { namespace cache {

#define TEST_CLASS LockFundCacheStorageTests

	TEST(TEST_CLASS, CanLoadValueIntoCache) {
		// Arrange:
		auto originalRecord = test::GenerateRecordGroup<LockFundHeightIndexDescriptor, test::DefaultRecordGroupGeneratorTraits<LockFundHeightIndexDescriptor>>(Height(10), 3);

		// Act:
		LockFundCache cache(CacheConfiguration{});
		{
			auto delta = cache.createDelta(Height{0});
			LockFundCacheStorage::LoadInto(originalRecord, *delta);
			cache.commit();
		}

		// Assert: the cache contains the value
		auto view = cache.createView(Height{0});
		EXPECT_EQ(1u, view->size());
		EXPECT_EQ(3u, view->secondarySize());
		ASSERT_TRUE(view->contains(Height(0)));
		for(auto& keyRecord : originalRecord.LockFundRecords)
		{
			ASSERT_TRUE(view->contains(keyRecord.first));
		}

		const auto& loadedHeightRecord = view->find(originalRecord.Identifier).get();
		auto records = test::DeriveKeyRecordsFromHeightRecord(originalRecord);
		for(auto& record : records)
		{
			const auto& loadedKeyRecord = view->find(record.first).get();
			test::AssertEqual(loadedKeyRecord, record.second);
		}
		// - the loaded cache value is correct
		test::AssertEqual(originalRecord, loadedHeightRecord);
	}
}}
