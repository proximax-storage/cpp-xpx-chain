/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/DbrbProcessEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS DbrbProcessEntryTests

	TEST(TEST_CLASS, CanCreateDbrbProcessEntry) {
		// Act:
		dbrb::ProcessId processId = test::GenerateRandomByteArray<Key>();
		Timestamp expirationTime = Timestamp(10);
		auto entry = DbrbProcessEntry(processId, expirationTime);

		// Assert:
		EXPECT_EQ(processId, entry.processId());
		EXPECT_EQ(expirationTime, entry.expirationTime());
	}

	TEST(TEST_CLASS, CanSetExpirationTime) {
		// Arrange:
		dbrb::ProcessId processId = test::GenerateRandomByteArray<Key>();
		Timestamp expirationTime = Timestamp(10);
		auto entry = DbrbProcessEntry(processId, expirationTime);

		// Sanity check:
		EXPECT_EQ(processId, entry.processId());
		EXPECT_EQ(expirationTime, entry.expirationTime());
		EXPECT_EQ(VersionType(1), entry.version());

		// Act:
		entry.setExpirationTime(Timestamp(20));

		// Assert:
		EXPECT_EQ(Timestamp(20), entry.expirationTime());
	}

}}
