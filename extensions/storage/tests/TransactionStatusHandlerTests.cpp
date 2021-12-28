/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/TestHarness.h"
#include "src/TransactionStatusHandler.h"

namespace catapult { namespace storage {

#define TEST_CLASS TransactionStatusHandlerTests

    TEST(TEST_CLASS, TransactionStatusHandler_CanAddHandler) {
        // Arrange:
        TransactionStatusHandler handler;

        // Act + Assert:
        EXPECT_NO_THROW(handler.addHandler(Hash256({ 1 }), [](uint32_t status) {}));
    }

    TEST(TEST_CLASS, TransactionStatusHandler_CanHandle) {
        // Arrange:
        TransactionStatusHandler handler;
        auto counter = 0;

		// Act:
        handler.addHandler(Hash256({ 1 }), [&counter](uint32_t status) { counter++; });
        handler.handle(Hash256({ 1 }), 0);

        // Assert:
        EXPECT_EQ(1, counter);
    }

    TEST(TEST_CLASS, TransactionStatusHandler_CannotHandleTwoTimes) {
        // Act:
        TransactionStatusHandler handler;
        auto counter = 0;

        handler.addHandler(Hash256({ 1 }), [&counter](uint32_t status) { counter++; });
        handler.handle(Hash256({ 1 }), 0);
        handler.handle(Hash256({ 1 }), 0);

        // Assert:
        EXPECT_EQ(1, counter);
    }
}}
