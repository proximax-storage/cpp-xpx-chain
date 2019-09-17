/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/config_holder/ConfigTreeCache.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace config {

#define TEST_CLASS ConfigTreeCacheTests

	// region insert

	TEST(TEST_CLASS, CanInsertConfig) {
		// Arrange:
		ConfigTreeCache testee;
		test::MutableBlockchainConfiguration config;
		config.Network.ImportanceGrouping = 5;

		// Act:
		auto& result = testee.insert(Height{777}, config.ToConst());

		// Assert:
		EXPECT_TRUE(testee.contains(Height{777}));
		EXPECT_EQ(5, result.Network.ImportanceGrouping);
	}

	TEST(TEST_CLASS, InsertThrowsWhenConfigExistsAtHeight) {
		// Arrange:
		ConfigTreeCache testee;
		test::MutableBlockchainConfiguration config;
		testee.insert(Height{777}, config.ToConst());
		// Sanity
		EXPECT_TRUE(testee.contains(Height{777}));

		// Act + Assert:
		EXPECT_THROW(testee.insert(Height{777}, config.ToConst()), catapult_invalid_argument);
	}

	// endregion

	// region insertRef

	TEST(TEST_CLASS, CanInsertRef) {
		// Arrange:
		ConfigTreeCache testee;
		test::MutableBlockchainConfiguration config;
		config.Network.ImportanceGrouping = 5;
		testee.insert(Height{777}, config.ToConst());
		// Sanity
		EXPECT_TRUE(testee.contains(Height{777}));

		// Act:
		auto& result = testee.insertRef(Height{555}, Height{777});

		// Assert:
		EXPECT_TRUE(testee.contains(Height{555}));
		EXPECT_EQ(5, result.Network.ImportanceGrouping);
	}

	TEST(TEST_CLASS, InsertRefThrowsOnRefAtConfigHeight) {
		// Arrange:
		ConfigTreeCache testee;

		// Act + Assert:
		EXPECT_THROW(testee.insertRef(Height{777}, Height{777}), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, InsertRefThrowsWhenConfigDoesntExistsAtHeight) {
		// Arrange:
		ConfigTreeCache testee;

		// Act + Assert:
		EXPECT_THROW(testee.insertRef(Height{555}, Height{777}), catapult_invalid_argument);
	}

	// endregion

	// region erase

	TEST(TEST_CLASS, CanEraseConfig) {
		// Arrange:
		ConfigTreeCache testee;
		test::MutableBlockchainConfiguration config;
		testee.insert(Height{777}, config.ToConst());
		// Sanity
		EXPECT_TRUE(testee.contains(Height{777}));

		// Act:
		testee.erase(Height{777});

		// Assert:
		EXPECT_FALSE(testee.contains(Height{777}));
	}

	TEST(TEST_CLASS, CanEraseRef) {
		// Arrange:
		ConfigTreeCache testee;
		test::MutableBlockchainConfiguration config;

		testee.insert(Height{777}, config.ToConst());
		// Sanity
		EXPECT_TRUE(testee.contains(Height{777}));

		testee.insertRef(Height{555}, Height{777});
		// Sanity
		EXPECT_TRUE(testee.contains(Height{555}));

		// Act:
		testee.erase(Height{555});

		// Assert:
		EXPECT_FALSE(testee.contains(Height{555}));
	}

	// region get

	TEST(TEST_CLASS, CanGetConfig) {
		// Arrange:
		ConfigTreeCache testee;
		test::MutableBlockchainConfiguration config;
		config.Network.ImportanceGrouping = 5;
		testee.insert(Height{777}, config.ToConst());
		// Sanity
		EXPECT_TRUE(testee.contains(Height{777}));

		// Act:
		auto& result = testee.get(Height{777});

		// Assert:
		EXPECT_EQ(5, result.Network.ImportanceGrouping);
	}

	TEST(TEST_CLASS, CanGetRef) {
		// Arrange:
		ConfigTreeCache testee;
		test::MutableBlockchainConfiguration config;
		config.Network.ImportanceGrouping = 5;

		testee.insert(Height{777}, config.ToConst());
		// Sanity
		EXPECT_TRUE(testee.contains(Height{777}));

		testee.insertRef(Height{555}, Height{777});
		// Sanity
		EXPECT_TRUE(testee.contains(Height{555}));

		// Act:
		auto& result = testee.get(Height{555});

		// Assert:
		EXPECT_EQ(5, result.Network.ImportanceGrouping);
	}

	TEST(TEST_CLASS, GetThrowsWhenConfigDoesntExistsAtHeight) {
		// Arrange:
		ConfigTreeCache testee;

		// Act + Assert:
		EXPECT_THROW(testee.get(Height{777}), catapult_invalid_argument);
	}

	// endregion
}}
