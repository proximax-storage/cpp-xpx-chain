///**
//*** Copyright 2021 ProximaX Limited. All rights reserved.
//*** Use of this source code is governed by the Apache 2.0
//*** license that can be found in the LICENSE file.
//**/
//
//#include "src/catapult/model/BlockUtils.cpp"
//#include "fastfinality/src/WeightedVotingFsm.h"
//#include "tests/test/core/BlockTestUtils.h"
//#include "tests/test/local/LocalTestUtils.h"
//#include "tests/test/core/ThreadPoolTestUtils.h"
//#include "tests/test/core/PacketPayloadTestUtils.h"
//#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
//
//namespace catapult { namespace fastfinality {
//
//#define TEST_CLASS TimeSyncHandlersTests
//
//	auto createValidRequest(const std::shared_ptr<model::Block>& pBlock, ionet::PacketType packetType) {
//		auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(pBlock->Size);
//		pPacket->Type = packetType;
//		std::memcpy(static_cast<void*>(pPacket.get() + 1), pBlock.get(), pBlock->Size);
//
//		return pPacket;
//	}
//
//	namespace {
//		template<typename THandler, typename TAct>
//		void RunPushHandlerTest(CommitteeStage& stage, THandler handlerFn, TAct actFunc) {
//			// Arrange:
//			std::shared_ptr<thread::IoThreadPool> pPool = test::CreateStartedIoThreadPool();
//			auto config = test::CreateUninitializedBlockchainConfiguration();
//			const_cast<utils::FileSize&>(config.Node.MaxPacketDataSize) = utils::FileSize::FromMegabytes(150);
//			std::shared_ptr<WeightedVotingFsm> pFsm = std::make_shared<WeightedVotingFsm>(pPool, config);
//			pFsm->committeeData().setCommitteeStage(stage);
//			ionet::ServerPacketHandlers handlers;
//			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
//			plugins::PluginManager pluginManager(pConfigHolder, plugins::StorageConfiguration());
//
//			// Register a handler
//			handlerFn(pFsm, handlers, &pluginManager);
//
//			// Act
//			actFunc(pFsm, handlers);
//		}
//	}
//
//	// region RegisterPullProposedBlockHandler
//
//	namespace {
//		template<typename TAssert>
//		void RunPushProposedBlockHandlerTest(
//				const std::shared_ptr<model::Block>& pBlock,
//				CommitteeStage stage,
//				TAssert assertFunc) {
//			RunPushHandlerTest(
//					stage,
//					[](auto pFsm, auto& handlers, auto* pluginManager) {
//						RegisterPullProposedBlockHandler(pFsm, handlers, *pluginManager);
//					},
//					[pBlock, assertFunc](auto pFsm, auto& handlers) {
//						auto pPacket = createValidRequest(pBlock, ionet::PacketType::Pull_Proposed_Block);
//
//						// Act:
//						ionet::ServerPacketHandlerContext context({}, "");
//						EXPECT_TRUE(handlers.process(*pPacket, context));
//
//						// Assert:
//						assertFunc(pFsm, handlers);
//						test::AssertNoResponse(context);
//					});
//		}
//	}
//
//	TEST(TEST_CLASS, PushProposedBlockHandler_SavesProposedBlock) {
//		// Arrange:
//		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//		RunPushProposedBlockHandlerTest(
//				pBlock,
//				CommitteeStage { 0u, CommitteePhase::Propose, utils::ToTimePoint(Timestamp()), 0u },
//				[pBlock](auto pFsm, auto handlers) {
//					// Assert:
//					ASSERT_EQ(*pBlock, *pFsm->committeeData().proposedBlock());
//				});
//	}
//
//	TEST(TEST_CLASS, PushProposedBlockHandler_RejectProposedBlock_Phase) {
//		// Arrange:
//		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//		RunPushProposedBlockHandlerTest(
//				pBlock,
//				CommitteeStage { 0u, CommitteePhase::None, utils::ToTimePoint(Timestamp()), 0u },
//				[pBlock](auto pFsm, auto handlers) {
//					// Assert:
//					ASSERT_EQ(nullptr, pFsm->committeeData().proposedBlock());
//				});
//	}
//
//	TEST(TEST_CLASS, PushProposedBlockHandler_RejectProposedBlock_PacketSize) {
//		// Arrange:
//		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//		pBlock->Size *= 2;
//		RunPushProposedBlockHandlerTest(
//				pBlock,
//				CommitteeStage { 0u, CommitteePhase::Propose, utils::ToTimePoint(Timestamp()), 0u },
//				[pBlock](auto pFsm, auto handlers) {
//					// Assert:
//					ASSERT_EQ(nullptr, pFsm->committeeData().proposedBlock());
//				});
//	}
//
//	TEST(TEST_CLASS, PushProposedBlockHandler_ExistProposed) {
//		// Arrange:
//		auto pExistingBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//		RunPushProposedBlockHandlerTest(
//				pExistingBlock,
//				CommitteeStage { 0u, CommitteePhase::Propose, utils::ToTimePoint(Timestamp()), 0u },
//				[pExistingBlock](auto pFsm, auto handlers) {
//					auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//					auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(pBlock->Size);
//					pPacket->Type = ionet::PacketType::Pull_Proposed_Block;
//					std::memcpy(static_cast<void*>(pPacket.get() + 1), pBlock.get(), pBlock->Size);
//
//					ionet::ServerPacketHandlerContext context({}, "");
//					EXPECT_TRUE(handlers.process(*pPacket, context));
//					ASSERT_EQ(*pExistingBlock, *pFsm->committeeData().proposedBlock());
//				});
//	}
//
//	// endregion
//
//	// region RegisterPullConfirmedBlockHandler
//
//	namespace {
//		template<typename TAssert>
//		void RunPushConfirmedBlockHandlerTest(
//				const std::shared_ptr<model::Block>& pBlock,
//				CommitteeStage stage,
//				TAssert assertFunc) {
//			RunPushHandlerTest(
//					stage,
//					[](auto pFsm, auto& handlers, auto* pluginManager) {
//						RegisterPullConfirmedBlockHandler(pFsm, handlers, *pluginManager);
//					},
//					[pBlock, assertFunc](auto pFsm, auto& handlers) {
//						auto pPacket = createValidRequest(pBlock, ionet::PacketType::Pull_Confirmed_Block);
//
//						// Act:
//						ionet::ServerPacketHandlerContext context({}, "");
//						EXPECT_TRUE(handlers.process(*pPacket, context));
//
//						// Assert:
//						assertFunc(pFsm, handlers);
//						test::AssertNoResponse(context);
//					});
//		}
//	}
//
//	TEST(TEST_CLASS, PushConfirmedBlockHandler_SavesConfirmedBlock) {
//		// Arrange:
//		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//		RunPushConfirmedBlockHandlerTest(
//				pBlock,
//				CommitteeStage { 0u, CommitteePhase::Commit, utils::ToTimePoint(Timestamp()), 0u },
//				[pBlock](auto pFsm, auto handlers) {
//					// Assert:
//					ASSERT_EQ(*pBlock, *pFsm->committeeData().confirmedBlock());
//				});
//	}
//
//	TEST(TEST_CLASS, PushConfirmedBlockHandler_RejectConfirmedBlock_Phase) {
//		// Arrange:
//		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//		RunPushConfirmedBlockHandlerTest(
//				pBlock,
//				CommitteeStage { 0u, CommitteePhase::None, utils::ToTimePoint(Timestamp()), 0u },
//				[pBlock](auto pFsm, auto handlers) {
//					// Assert:
//					ASSERT_EQ(nullptr, pFsm->committeeData().confirmedBlock());
//				});
//	}
//
//	TEST(TEST_CLASS, PushConfirmedBlockHandler_RejectConfirmedBlock_PacketSize) {
//		// Arrange:
//		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//		pBlock->Size *= 2;
//		RunPushConfirmedBlockHandlerTest(
//				pBlock,
//				CommitteeStage { 0u, CommitteePhase::Commit, utils::ToTimePoint(Timestamp()), 0u },
//				[pBlock](auto pFsm, auto handlers) {
//					// Assert:
//					ASSERT_EQ(nullptr, pFsm->committeeData().confirmedBlock());
//				});
//	}
//
//	TEST(TEST_CLASS, PushConfirmedBlockHandler_ExistConfirmed) {
//		// Arrange:
//		auto pExistingBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//		RunPushConfirmedBlockHandlerTest(
//				pExistingBlock,
//				CommitteeStage { 0u, CommitteePhase::Commit, utils::ToTimePoint(Timestamp()), 0u },
//				[pExistingBlock](auto pFsm, auto handlers) {
//					auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//					auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(pBlock->Size);
//					pPacket->Type = ionet::PacketType::Pull_Confirmed_Block;
//					std::memcpy(static_cast<void*>(pPacket.get() + 1), pBlock.get(), pBlock->Size);
//
//					ionet::ServerPacketHandlerContext context({}, "");
//					EXPECT_TRUE(handlers.process(*pPacket, context));
//					ASSERT_EQ(*pExistingBlock, *pFsm->committeeData().confirmedBlock());
//				});
//	}
//
//	// endregion
//
//	// region RegisterPullVoteMessagesHandler
//
//	template<typename TMessage>
//	void makePacketValid(const std::shared_ptr<TMessage>& pPacket, const Key& pubKey, const Hash256& blockHash) {
//		pPacket->Type = ionet::PacketType::Pull_Prevote_Messages;
//		pPacket->Size = sizeof(TMessage);
//		pPacket->Message.BlockHash = blockHash;
//		pPacket->Message.BlockCosignature.Signer = pubKey;
//	}
//
//	namespace {
//		template<typename TAssert, typename TAct>
//		void RunPushVoteMessageHandlerTest(TAct actFunc, TAssert assertFunc) {
//			const auto keyPair = crypto::KeyPair::FromPrivate(test::GenerateRandomPrivateKey());
//			std::shared_ptr<thread::IoThreadPool> pPool = test::CreateStartedIoThreadPool();
//			auto config = test::CreateUninitializedBlockchainConfiguration();
//			const_cast<utils::FileSize&>(config.Node.MaxPacketDataSize) = utils::FileSize::FromMegabytes(150);
//			std::shared_ptr<WeightedVotingFsm> pFsm = std::make_shared<WeightedVotingFsm>(pPool, config);
//			ionet::ServerPacketHandlers handlers;
//			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
//			plugins::PluginManager pluginManager(pConfigHolder, plugins::StorageConfiguration());
//
//			// Act:
//			actFunc(keyPair, pFsm, handlers, pluginManager);
//
//			// Assert:
//			assertFunc(pFsm);
//		}
//	}
//
//	TEST(TEST_CLASS, RegisterPushPrevoteMessageHandler_Success) {
//		auto stage = CommitteeStage { 0u, CommitteePhase::Prevote, utils::ToTimePoint(Timestamp()), 0u };
//		auto pPacket = std::make_shared<PullPrevoteMessagesRequest>();
//		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//
//		RunPushVoteMessageHandlerTest(
//				// Act func
//				[&stage, pPacket, pBlock](const auto& keyPair, auto pFsm, auto handlers, auto& pluginManager) {
//					auto initStagePhase = stage.Phase;
//					stage.Phase = CommitteePhase::Propose;
//					pFsm->committeeData().setCommitteeStage(stage);
//
//					RegisterPullProposedBlockHandler(pFsm, handlers, pluginManager);
//					RegisterPullPrevoteMessagesHandler(pFsm, handlers);
//
//					crypto::Sign(
//							keyPair, model::BlockDataBuffer((*pBlock)), pPacket->Message.BlockCosignature.Signature);
//					auto pBlockPacket = createValidRequest(pBlock, ionet::PacketType::Pull_Proposed_Block);
//					ionet::ServerPacketHandlerContext context({}, "");
//					EXPECT_TRUE(handlers.process(*pBlockPacket, context));
//
//					stage.Phase = initStagePhase;
//					pFsm->committeeData().setCommitteeStage(stage);
//
//					makePacketValid(pPacket, keyPair.publicKey(), pFsm->committeeData().proposedBlockHash());
//					crypto::Sign(keyPair, CommitteeMessageDataBuffer(*pPacket), pPacket->MessageSignature);
//					EXPECT_TRUE(handlers.process(*pPacket, context));
//				},
//				// Assert func
//				[pPacket](auto pFsm) {
//					ASSERT_TRUE(pFsm->committeeData().hasVote(
//							pPacket->Message.BlockCosignature.Signer, pPacket->Message_Type));
//				});
//	}
//
//	TEST(TEST_CLASS, RegisterPushPrevoteMessageHandler_BadPhase) {
//		auto pPacket = std::make_shared<PullPrevoteMessagesRequest>();
//		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//		auto stage = CommitteeStage { 0u, CommitteePhase::Prevote, utils::ToTimePoint(Timestamp()), 0u };
//
//		RunPushVoteMessageHandlerTest(
//				// Act func
//				[&stage, pPacket, pBlock](const auto& keyPair, auto pFsm, auto handlers, auto& pluginManager) {
//					stage.Phase = CommitteePhase::Propose;
//					pFsm->committeeData().setCommitteeStage(stage);
//
//					RegisterPullProposedBlockHandler(pFsm, handlers, pluginManager);
//					RegisterPullPrevoteMessagesHandler(pFsm, handlers);
//
//					crypto::Sign(
//							keyPair, model::BlockDataBuffer((*pBlock)), pPacket->Message.BlockCosignature.Signature);
//					auto pBlockPacket = createValidRequest(pBlock, ionet::PacketType::Pull_Proposed_Block);
//					ionet::ServerPacketHandlerContext context({}, "");
//					EXPECT_TRUE(handlers.process(*pBlockPacket, context));
//
//					makePacketValid(pPacket, keyPair.publicKey(), pFsm->committeeData().proposedBlockHash());
//					crypto::Sign(keyPair, CommitteeMessageDataBuffer(*pPacket), pPacket->MessageSignature);
//					EXPECT_TRUE(handlers.process(*pPacket, context));
//				},
//				// Assert func
//				[pPacket](auto pFsm) {
//					ASSERT_FALSE(pFsm->committeeData().hasVote(
//							pPacket->Message.BlockCosignature.Signer, pPacket->Message_Type));
//				});
//	}
//
//	TEST(TEST_CLASS, RegisterPushPrevoteMessageHandler_InvalidPacket) {
//		auto stage = CommitteeStage { 0u, CommitteePhase::Prevote, utils::ToTimePoint(Timestamp()), 0u };
//		auto pPacket = std::make_shared<PullPrevoteMessagesRequest>();
//		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//
//		RunPushVoteMessageHandlerTest(
//				// Act func
//				[&stage, pPacket, pBlock](const auto& keyPair, auto pFsm, auto handlers, auto& pluginManager) {
//					auto initStagePhase = stage.Phase;
//					stage.Phase = CommitteePhase::Propose;
//					pFsm->committeeData().setCommitteeStage(stage);
//
//					RegisterPullProposedBlockHandler(pFsm, handlers, pluginManager);
//					RegisterPullPrevoteMessagesHandler(pFsm, handlers);
//
//					crypto::Sign(
//							keyPair, model::BlockDataBuffer((*pBlock)), pPacket->Message.BlockCosignature.Signature);
//					auto pBlockPacket = createValidRequest(pBlock, ionet::PacketType::Pull_Proposed_Block);
//					ionet::ServerPacketHandlerContext context({}, "");
//					EXPECT_TRUE(handlers.process(*pBlockPacket, context));
//
//					stage.Phase = initStagePhase;
//					pFsm->committeeData().setCommitteeStage(stage);
//
//					makePacketValid(pPacket, keyPair.publicKey(), pFsm->committeeData().proposedBlockHash());
//					pPacket->Size = 0;
//					crypto::Sign(keyPair, CommitteeMessageDataBuffer(*pPacket), pPacket->MessageSignature);
//					EXPECT_TRUE(handlers.process(*pPacket, context));
//				},
//				// Assert func
//				[pPacket](auto pFsm) {
//					ASSERT_FALSE(pFsm->committeeData().hasVote(
//							pPacket->Message.BlockCosignature.Signer, pPacket->Message_Type));
//				});
//	}
//
//	TEST(TEST_CLASS, RegisterPushPrevoteMessageHandler_AlreadyHasVote) {
//		auto stage = CommitteeStage { 0u, CommitteePhase::Prevote, utils::ToTimePoint(Timestamp()), 0u };
//		auto pPacket = std::make_shared<PullPrevoteMessagesRequest>();
//		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//
//		RunPushVoteMessageHandlerTest(
//				// Act func
//				[&stage, pPacket, pBlock](const auto& keyPair, auto pFsm, auto handlers, auto& pluginManager) {
//					auto pExistPacket = std::make_shared<PullPrevoteMessagesRequest>();
//					pExistPacket->Message.BlockCosignature.Signer = keyPair.publicKey();
//					pFsm->committeeData().addVote(std::move(pExistPacket));
//
//					auto initStagePhase = stage.Phase;
//					stage.Phase = CommitteePhase::Propose;
//					pFsm->committeeData().setCommitteeStage(stage);
//
//					RegisterPullProposedBlockHandler(pFsm, handlers, pluginManager);
//					RegisterPullPrevoteMessagesHandler(pFsm, handlers);
//
//					crypto::Sign(
//							keyPair, model::BlockDataBuffer((*pBlock)), pPacket->Message.BlockCosignature.Signature);
//					auto pBlockPacket = createValidRequest(pBlock, ionet::PacketType::Pull_Proposed_Block);
//					ionet::ServerPacketHandlerContext context({}, "");
//					EXPECT_TRUE(handlers.process(*pBlockPacket, context));
//
//					stage.Phase = initStagePhase;
//					pFsm->committeeData().setCommitteeStage(stage);
//
//					makePacketValid(pPacket, keyPair.publicKey(), pFsm->committeeData().proposedBlockHash());
//					crypto::Sign(keyPair, CommitteeMessageDataBuffer(*pPacket), pPacket->MessageSignature);
//					EXPECT_TRUE(handlers.process(*pPacket, context));
//				},
//				// Assert func
//				[pPacket](auto pFsm) {
//					EXPECT_TRUE(pFsm->committeeData().hasVote(
//							pPacket->Message.BlockCosignature.Signer, pPacket->Message_Type));
//					auto el = pFsm->committeeData().prevotes().find(pPacket->Message.BlockCosignature.Signer);
//					ASSERT_NE(el->second->Message.BlockHash, pPacket->Message.BlockHash);
//				});
//	}
//
//	TEST(TEST_CLASS, RegisterPushPrevoteMessageHandler_NoProposedBlock) {
//		auto stage = CommitteeStage { 0u, CommitteePhase::Prevote, utils::ToTimePoint(Timestamp()), 0u };
//		auto pPacket = std::make_shared<PullPrevoteMessagesRequest>();
//		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//
//		RunPushVoteMessageHandlerTest(
//				// Act func
//				[&stage, pPacket, pBlock](const auto& keyPair, auto pFsm, auto handlers, auto& pluginManager) {
//					pFsm->committeeData().setCommitteeStage(stage);
//					RegisterPullPrevoteMessagesHandler(pFsm, handlers);
//
//					makePacketValid(pPacket, keyPair.publicKey(), pFsm->committeeData().proposedBlockHash());
//					crypto::Sign(keyPair, CommitteeMessageDataBuffer(*pPacket), pPacket->MessageSignature);
//					ionet::ServerPacketHandlerContext context({}, "");
//					EXPECT_TRUE(handlers.process(*pPacket, context));
//				},
//				// Assert func
//				[pPacket](auto pFsm) {
//					ASSERT_FALSE(pFsm->committeeData().hasVote(
//							pPacket->Message.BlockCosignature.Signer, pPacket->Message_Type));
//				});
//	}
//
//	TEST(TEST_CLASS, RegisterPushPrevoteMessageHandler_BadBlockHash) {
//		auto stage = CommitteeStage { 0u, CommitteePhase::Prevote, utils::ToTimePoint(Timestamp()), 0u };
//		auto pPacket = std::make_shared<PullPrevoteMessagesRequest>();
//		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//
//		RunPushVoteMessageHandlerTest(
//				// Act func
//				[&stage, pPacket, pBlock](const auto& keyPair, auto pFsm, auto handlers, auto& pluginManager) {
//					RegisterPullProposedBlockHandler(pFsm, handlers, pluginManager);
//					RegisterPullPrevoteMessagesHandler(pFsm, handlers);
//
//					auto initStagePhase = stage.Phase;
//					stage.Phase = CommitteePhase::Propose;
//					pFsm->committeeData().setCommitteeStage(stage);
//
//					crypto::Sign(
//							keyPair, model::BlockDataBuffer((*pBlock)), pPacket->Message.BlockCosignature.Signature);
//					auto pBlockPacket = createValidRequest(pBlock, ionet::PacketType::Pull_Proposed_Block);
//					ionet::ServerPacketHandlerContext context({}, "");
//					EXPECT_TRUE(handlers.process(*pBlockPacket, context));
//
//					stage.Phase = initStagePhase;
//					pFsm->committeeData().setCommitteeStage(stage);
//
//					makePacketValid(pPacket, keyPair.publicKey(), test::GenerateRandomByteArray<Hash256>());
//					crypto::Sign(keyPair, CommitteeMessageDataBuffer(*pPacket), pPacket->MessageSignature);
//					EXPECT_TRUE(handlers.process(*pPacket, context));
//				},
//				// Assert func
//				[pPacket](auto pFsm) {
//					ASSERT_FALSE(pFsm->committeeData().hasVote(
//							pPacket->Message.BlockCosignature.Signer, pPacket->Message_Type));
//				});
//	}
//
//	TEST(TEST_CLASS, RegisterPushPrevoteMessageHandler_BadMessageSignature) {
//		auto stage = CommitteeStage { 0u, CommitteePhase::Prevote, utils::ToTimePoint(Timestamp()), 0u };
//		auto pPacket = std::make_shared<PullPrevoteMessagesRequest>();
//		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//
//		RunPushVoteMessageHandlerTest(
//				// Act func
//				[&stage, pPacket, pBlock](const auto& keyPair, auto pFsm, auto handlers, auto& pluginManager) {
//					RegisterPullProposedBlockHandler(pFsm, handlers, pluginManager);
//					RegisterPullPrevoteMessagesHandler(pFsm, handlers);
//
//					auto initStagePhase = stage.Phase;
//					stage.Phase = CommitteePhase::Propose;
//					pFsm->committeeData().setCommitteeStage(stage);
//					stage.Phase = CommitteePhase::Commit;
//
//					crypto::Sign(
//							keyPair, model::BlockDataBuffer((*pBlock)), pPacket->Message.BlockCosignature.Signature);
//					auto pBlockPacket = createValidRequest(pBlock, ionet::PacketType::Pull_Proposed_Block);
//					ionet::ServerPacketHandlerContext context({}, "");
//					EXPECT_TRUE(handlers.process(*pBlockPacket, context));
//
//					stage.Phase = initStagePhase;
//					pFsm->committeeData().setCommitteeStage(stage);
//
//					makePacketValid(pPacket, keyPair.publicKey(), pFsm->committeeData().proposedBlockHash());
//					EXPECT_TRUE(handlers.process(*pPacket, context));
//				},
//				// Assert func
//				[pPacket](auto pFsm) {
//					ASSERT_FALSE(pFsm->committeeData().hasVote(
//							pPacket->Message.BlockCosignature.Signer, pPacket->Message_Type));
//				});
//	}
//
//	TEST(TEST_CLASS, RegisterPushPrevoteMessageHandler_BadBlockHeaderCosignature) {
//		auto stage = CommitteeStage { 0u, CommitteePhase::Prevote, utils::ToTimePoint(Timestamp()), 0u };
//		auto pPacket = std::make_shared<PullPrevoteMessagesRequest>();
//		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
//
//		RunPushVoteMessageHandlerTest(
//				// Act func
//				[&stage, pPacket, pBlock](const auto& keyPair, auto pFsm, auto handlers, auto& pluginManager) {
//					RegisterPullProposedBlockHandler(pFsm, handlers, pluginManager);
//					RegisterPullPrevoteMessagesHandler(pFsm, handlers);
//
//					auto initStagePhase = stage.Phase;
//					stage.Phase = CommitteePhase::Propose;
//					pFsm->committeeData().setCommitteeStage(stage);
//
//					auto pBlockPacket = createValidRequest(pBlock, ionet::PacketType::Pull_Proposed_Block);
//					ionet::ServerPacketHandlerContext context({}, "");
//					EXPECT_TRUE(handlers.process(*pBlockPacket, context));
//
//					stage.Phase = initStagePhase;
//					pFsm->committeeData().setCommitteeStage(stage);
//
//					makePacketValid(pPacket, keyPair.publicKey(), pFsm->committeeData().proposedBlockHash());
//					crypto::Sign(keyPair, CommitteeMessageDataBuffer(*pPacket), pPacket->MessageSignature);
//					EXPECT_TRUE(handlers.process(*pPacket, context));
//				},
//				// Assert func
//				[pPacket](auto pFsm) {
//					ASSERT_FALSE(pFsm->committeeData().hasVote(
//							pPacket->Message.BlockCosignature.Signer, pPacket->Message_Type));
//				});
//	}
//
//	// endregion
//}}
