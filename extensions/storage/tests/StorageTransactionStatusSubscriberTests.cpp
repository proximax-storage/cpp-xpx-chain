/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "storage/src/StorageTransactionStatusSubscriber.h"
#include "tests/TestHarness.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "src/ReplicatorService.h"

namespace catapult { namespace storage {

#define TEST_CLASS StorageTransactionStatusSubscriberTests

    TEST(TEST_CLASS, CanCreateStorageTransactionStatusSubscriber) {
        // Act:
        auto config = config::CreateMockConfigurationHolder();
        auto storageConfig = StorageConfiguration::Uninitialized();
        storageConfig.Key = "0000000000000000000000000000000000000000000000000000000000000000";
        std::vector<ionet::Node> bootstrapReplicators;

        auto pReplicatorService = std::make_shared<ReplicatorService>(
                std::move(storageConfig),
                std::move(bootstrapReplicators));
        auto testee = CreateStorageTransactionStatusSubscriber(pReplicatorService);

        // Assert:
        EXPECT_NE(nullptr, testee);
    }
}}
