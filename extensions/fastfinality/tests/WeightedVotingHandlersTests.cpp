/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/ionet/PacketPayloadFactory.h"
#include "fastfinality/src/WeightedVotingFsm.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/PacketPayloadTestUtils.h"
#include "tests/test/core/ThreadPoolTestUtils.h"
#include "tests/test/local/LocalTestUtils.h"

namespace catapult { namespace fastfinality {

#define TEST_CLASS TimeSyncHandlersTests

	// region RegisterPushProposedBlockHandler

	namespace {
		template<typename TAssert>
		void RunPushProposedBlockHandlerTest(const std::shared_ptr<model::Block>& pBlock, TAssert assertFunc) {
			// Arrange:
			std::shared_ptr<thread::IoThreadPool> pPool = test::CreateStartedIoThreadPool();
			std::shared_ptr<WeightedVotingFsm> pFsm = std::make_shared<WeightedVotingFsm>(pPool);
			pFsm->committeeData().setCommitteeStage(CommitteeStage{
				0u,
				CommitteePhase::Propose,
				utils::ToTimePoint(Timestamp()),
				0u
			});
			ionet::ServerPacketHandlers handlers;
			auto config = test::CreateUninitializedBlockchainConfiguration();
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			plugins::PluginManager pluginManager(pConfigHolder, plugins::StorageConfiguration());

			RegisterPushProposedBlockHandler(pFsm, handlers, pluginManager);

			// - create a valid request
			auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(pBlock->Size);
			pPacket->Type = ionet::PacketType::Push_Proposed_Block;
			std::memcpy(static_cast<void*>(pPacket.get() + 1), pBlock.get(), pBlock->Size);

			// Act:
			ionet::ServerPacketHandlerContext context({}, "");
			EXPECT_TRUE(handlers.process(*pPacket, context));

			// Assert:
			assertFunc(pFsm);
			test::AssertNoResponse(context);
		}
	}

	TEST(TEST_CLASS, PushProposedBlockHandler_SavesProposedBlock) {
		// Arrange:
		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
		RunPushProposedBlockHandlerTest(pBlock, [pBlock](auto pFsm) {
			// Assert:
			ASSERT_EQ(*pBlock, *pFsm->committeeData().proposedBlock());
		});
	}

	// endregion
}}
