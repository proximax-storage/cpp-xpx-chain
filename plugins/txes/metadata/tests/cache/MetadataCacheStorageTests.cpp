/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/MetadataCacheStorage.h"
#include "src/cache/MetadataCache.h"
#include "src/model/MetadataTypes.h"
#include "tests/test/MetadataCacheTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS MetadataCacheStorageTests

	TEST(TEST_CLASS, CanLoadValueIntoCache) {
		// Arrange:
		state::MetadataEntry originalMetadataEntry(state::ToVector(test::GenerateRandomData<Address_Decoded_Size>()), model::MetadataType{0});
		for (auto i = 0u; i < 3; ++i)
			originalMetadataEntry.fields().push_back(state::MetadataField{ "Hello", "World", Height(1 + i) });

		// Sanity:
		EXPECT_EQ(3u, originalMetadataEntry.fields().size());

		// Act:
		MetadataCache cache(CacheConfiguration{});
		{
			auto delta = cache.createDelta();
			MetadataCacheStorage::LoadInto(originalMetadataEntry, *delta);
			cache.commit();
		}

		// Assert: the cache contains the value
		auto view = cache.createView();
		EXPECT_EQ(1u, view->size());
		ASSERT_TRUE(view->contains(originalMetadataEntry.metadataId()));
		const auto& loadedMetadataEntry = view->find(originalMetadataEntry.metadataId()).get();

		// - the loaded cache value is correct
		test::AssertEqual(originalMetadataEntry, loadedMetadataEntry);
	}
}}
