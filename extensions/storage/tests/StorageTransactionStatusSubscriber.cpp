/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "storage/src/StorageTransactionStatusSubscriber.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/TestHarness.h"
#include "src/StorageConfiguration.h"
#include "src/ReplicatorService.h"

namespace catapult { namespace storage {

#define TEST_CLASS StorageTransactionStatusSubscriberTests

    TEST(TEST_CLASS, CanCreateStorageTransactionStatusSubscriber) {
        // Act:
        auto config = config::CreateMockConfigurationHolder();
        auto keyPair = crypto::KeyPair::FromString("test");
        auto storageConfig = StorageConfiguration{};

        auto pReplicatorService = std::make_shared<ReplicatorService>(
                std::move(keyPair),
                std::move(storageConfig));
        auto testee = CreateStorageTransactionStatusSubscriber(pReplicatorService);

        // Assert:
        EXPECT_NE(nullptr, testee);
    }
}}
