/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/TestHarness.h"
#include "src/StorageConfiguration.h"
#include "src/TransactionStatusHandler.h"

namespace catapult { namespace storage {

#define TEST_CLASS TransactionStatusHandlerTests

    TEST(TEST_CLASS, TransactionStatusHandler_CanAddHandler) {
        // Act:
        TransactionStatusHandler handler{};
        Signature signature{};

        // Assert:
        EXPECT_ANY_THROW(handler.addHandler(signature, [](const Hash256& hash, uint32_t status){}));
    }

    TEST(TEST_CLASS, TransactionStatusHandler_CanHandle) {
        // Act:
        TransactionStatusHandler handler{};
        Signature signature{};
        auto counter = 0;

        handler.addHandler(signature, [&counter](const Hash256& hash, uint32_t status){ counter++; });
        handler.handle(signature, Hash256{}, 0);

        // Assert:
        EXPECT_EQ(1, counter);
    }

    TEST(TEST_CLASS, TransactionStatusHandler_CannotHandleTwoTimes) {
        // Act:
        TransactionStatusHandler handler{};
        Signature signature{};
        auto counter = 0;

        handler.addHandler(signature, [&counter](const Hash256& hash, uint32_t status){ counter++; });
        handler.handle(signature, Hash256{}, 0);
        handler.handle(signature, Hash256{}, 0);

        // Assert:
        EXPECT_EQ(1, counter);
    }
}}
