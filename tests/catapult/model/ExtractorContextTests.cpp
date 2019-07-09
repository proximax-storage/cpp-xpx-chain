/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/ExtractorContext.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

#define TEST_CLASS ExtractorContextTests

	TEST(TEST_CLASS, CanExtractAddresses_DefaultExtractor) {
		// Arrange:
		ExtractorContext context;

		// Act:
		auto result = context.extract(UnresolvedAddress{ { { 123 } } });

		// Assert:
		EXPECT_EQ(UnresolvedAddressSet{ { { { 123 } } } }, result);
	}

	TEST(TEST_CLASS, CanExtractPublicKeys_DefaultExtractor) {
		// Arrange:
		ExtractorContext context;

		// Act:
		auto result = context.extract(Key{ { { 123 } }});

		// Assert:
		EXPECT_EQ(PublicKeySet{ { { { 123 } } } }, result);
	}

	TEST(TEST_CLASS, CanExtractAddresses_CustomExtractor) {
		// Arrange:
		ExtractorContext context(
				[](const auto& address) { return UnresolvedAddressSet{ address, address }; },
				[](const auto&) { return PublicKeySet{}; });

		// Act:
		auto result = context.extract(UnresolvedAddress{ { { 123 } } });

		// Assert:
		EXPECT_EQ((UnresolvedAddressSet{ { { { 123 } } }, { { { 123 } } } }), result);
	}

	TEST(TEST_CLASS, CanExtractPublicKeys_CustomExtractor) {
		// Arrange:
		ExtractorContext context(
				[](const auto&) { return UnresolvedAddressSet{}; },
				[](const auto& key) { return PublicKeySet{ key, key }; });

		// Act:
		auto result = context.extract(Key{ { { 123 } } });

		// Assert:
		EXPECT_EQ((PublicKeySet{ { { { 123 } } }, { { { 123 } } } }), result);
	}
}}
