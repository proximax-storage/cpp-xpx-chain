/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/ReplicatorService.h"
#include "tests/TestHarness.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/other/mocks/MockServiceState.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"

namespace catapult { namespace storage {

#define TEST_CLASS ReplicatorServiceTests

    TEST(TEST_CLASS, CanCreateReplicatorService) {
        // Act:
        auto config = config::CreateMockConfigurationHolder();
        auto keyPair = test::GenerateKeyPair();
        StorageConfiguration storageConfig{};

        auto pReplicatorService = std::make_shared<ReplicatorService>(
                std::move(keyPair),
                std::move(storageConfig)
        );

        auto serviceState = mocks::CreateMockServiceState();
        pReplicatorService->setServiceState(serviceState->ServiceState());

        // Assert:
        EXPECT_NO_THROW(pReplicatorService->start());
        EXPECT_NO_THROW(pReplicatorService->stop());
    }
}}
