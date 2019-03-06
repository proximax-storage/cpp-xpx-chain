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

#include "catapult/disruptor/DisruptorTypes.h"
#include "tests/TestHarness.h"

namespace catapult { namespace disruptor {

#define TEST_CLASS DisruptorTypesTests

	// region ConsumerResult

	TEST(TEST_CLASS, CanCreateDefaultConsumerResult) {
		// Act:
		auto result = ConsumerResult();

		// Assert:
		EXPECT_EQ(CompletionStatus::Normal, result.CompletionStatus);
		EXPECT_EQ(0u, result.CompletionCode);
	}

	TEST(TEST_CLASS, CanCreateContinueConsumerResult) {
		// Act:
		auto result = ConsumerResult::Continue();

		// Assert:
		EXPECT_EQ(CompletionStatus::Normal, result.CompletionStatus);
		EXPECT_EQ(0u, result.CompletionCode);
	}

	TEST(TEST_CLASS, CanCreateAbortConsumerResult) {
		// Act:
		auto result = ConsumerResult::Abort();

		// Assert:
		EXPECT_EQ(CompletionStatus::Aborted, result.CompletionStatus);
		EXPECT_EQ(0u, result.CompletionCode);
	}

	TEST(TEST_CLASS, CanCreateAbortConsumerResultWithCustomCompletionCode) {
		// Act:
		auto result = ConsumerResult::Abort(456);

		// Assert:
		EXPECT_EQ(CompletionStatus::Aborted, result.CompletionStatus);
		EXPECT_EQ(456u, result.CompletionCode);
	}

	TEST(TEST_CLASS, CanCreateCompleteConsumerResultWithCustomCompletionCode) {
		// Act:
		auto result = ConsumerResult::Complete(456);

		// Assert:
		EXPECT_EQ(CompletionStatus::Consumed, result.CompletionStatus);
		EXPECT_EQ(456u, result.CompletionCode);
	}

	// endregion

	// region ConsumerCompletionResult

	TEST(TEST_CLASS, CanCreateDefaultConsumerCompletionResult) {
		// Act:
		auto result = ConsumerCompletionResult();

		// Assert:
		EXPECT_EQ(CompletionStatus::Normal, result.CompletionStatus);
		EXPECT_EQ(0u, result.CompletionCode);
		EXPECT_EQ(std::numeric_limits<uint64_t>::max(), result.FinalConsumerPosition);
	}

	// endregion
}}
