/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/DbrbTestUtils.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"

namespace catapult { namespace cache {

#define TEST_CLASS ViewSequenceCacheTests

	namespace {
		struct ViewSequenceCacheMixinTraits {
			class CacheType : public ViewSequenceCache {
			public:
				CacheType() : ViewSequenceCache(CacheConfiguration(), config::CreateMockConfigurationHolder())
				{}
			};
		};
	}

	// region custom tests

	TEST(TEST_CLASS, LatestViewIsUpdatedCorrectly) {
		// Arrange:
		ViewSequenceCacheMixinTraits::CacheType cache;

		// Initial state assert:
		{
			auto view = cache.createView(Height(0));
			EXPECT_EQ(0, view->size());
			EXPECT_EQ(dbrb::View{}, view->getLatestView());
		}

		// Inserting first sequence.
		{
			// Act:
			const auto hash = test::GenerateRandomByteArray<Hash256>();
			const auto sequence = test::GenerateRandomSequence();

			auto delta = cache.createDelta(Height(0));
			delta->insert(test::CreateViewSequenceEntry(hash, sequence));
			cache.commit();

			// Assert:
			auto view = cache.createView(Height(0));
			EXPECT_EQ(1, view->size());

			const auto& entry = view->find(hash).get();
			EXPECT_EQ(sequence, entry.sequence());

			const auto& sequenceLatestView = sequence.maybeMostRecent().value();
			EXPECT_EQ(sequenceLatestView, view->getLatestView());
		}

		// Inserting second sequence.
		{
			// Act:
			const auto hash = test::GenerateRandomByteArray<Hash256>();
			const auto sequence = test::GenerateRandomSequence();

			auto delta = cache.createDelta(Height(1));
			delta->insert(test::CreateViewSequenceEntry(hash, sequence));
			cache.commit();

			// Assert:
			auto view = cache.createView(Height(1));
			EXPECT_EQ(2, view->size());

			const auto& entry = view->find(hash).get();
			EXPECT_EQ(sequence, entry.sequence());

			const auto& sequenceLatestView = sequence.maybeMostRecent().value();
			EXPECT_EQ(sequenceLatestView, view->getLatestView());
		}
	}

	// endregion

}}

