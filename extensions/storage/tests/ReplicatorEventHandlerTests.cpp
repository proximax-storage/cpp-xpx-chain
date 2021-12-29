/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/ReplicatorEventHandler.h"
#include "src/TransactionSender.h"
#include "src/TransactionStatusHandler.h"
#include "tests/TestHarness.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/mocks/MockStorageState.h"

namespace catapult { namespace storage {

#define TEST_CLASS ReplicatorEventHandlerTests

    TEST(TEST_CLASS, CanCreateReplicatorEventHandler) {
        // Arrange:
        auto config = config::CreateMockConfigurationHolder();
        mocks::MockStorageState storageState;
        StorageConfiguration storageConfig{};
        handlers::TransactionRangeHandler transactionRangeHandler{};
        auto transactionSender = TransactionSender(
			crypto::KeyPair::FromString("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"),
			config->Config().Immutable,
			storageConfig,
			transactionRangeHandler);
        TransactionStatusHandler transactionStatusHandler;

        // Act:
        auto testee = storage::CreateReplicatorEventHandler(
			std::move(transactionSender),
			(state::StorageState&) storageState,
			transactionStatusHandler
        );

        // Assert:
        EXPECT_NE(nullptr, testee);
    }
}}
