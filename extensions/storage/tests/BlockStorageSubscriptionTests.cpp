/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "storage/src/BlockStorageSubscription.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/TestHarness.h"

namespace catapult { namespace storage {

#define TEST_CLASS BlockStorageSubscriptionTests

	TEST(TEST_CLASS, CanCreateBlockStorageSubscription) {
		// Act:
		extensions::ProcessBootstrapper bootstrapper(
			config::CreateMockConfigurationHolder(),
			"",
			extensions::ProcessDisposition::Production,
			"");
		HandlerPointer pHandler;
		auto testee = CreateBlockStorageSubscription(bootstrapper, pHandler);

		// Assert:
		EXPECT_NE(nullptr, testee);
	}
}}
