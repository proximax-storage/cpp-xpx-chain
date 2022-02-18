///**
//*** Copyright 2021 ProximaX Limited. All rights reserved.
//*** Use of this source code is governed by the Apache 2.0
//*** license that can be found in the LICENSE file.
//**/
//
//#include "src/ReplicatorService.h"
//#include "tests/test/other/mocks/MockServiceState.h"
//
//namespace catapult { namespace storage {
//
//#define TEST_CLASS ReplicatorServiceTests
//
//    TEST(TEST_CLASS, CanCreateReplicatorService) {
//        // Act:
//        auto config = config::CreateMockConfigurationHolder();
//        auto keyPair = test::GenerateKeyPair();
//        auto storageConfig = StorageConfiguration::Uninitialized();
//		std::vector<ionet::Node> bootstrapReplicators;
//
//        auto pReplicatorService = std::make_shared<ReplicatorService>(
//                std::move(keyPair),
//                std::move(storageConfig),
//				std::move(bootstrapReplicators));
//
//        mocks::MockServiceState serviceState{};
//        pReplicatorService->setServiceState(serviceState.ServiceState());
//
//        // Assert:
//        EXPECT_NO_THROW(pReplicatorService->start());
//        EXPECT_NO_THROW(pReplicatorService->stop());
//    }
//}}
