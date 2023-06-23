/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/catapult/extensions/NetworkUtils.h"
#include "catapult/ionet/PacketPayloadFactory.h"
#include "fastfinality/src/WeightedVotingFsm.h"
#include "fastfinality/src/WeightedVotingService.h"
#include "tests/test/local/ServiceLocatorTestContext.h"
#include "tests/test/core/ThreadPoolTestUtils.h"
#include "tests/test/core/PacketPayloadTestUtils.h"
#include "tests/test/core/mocks/MockDbrbViewFetcher.h"
#include "tests/test/core/mocks/MockCommitteeManager.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/net/mocks/MockPacketWriters.h"
#include "fastfinality/src/utils/WeightedVotingUtils.h"
#include "catapult/crypto/Signer.h"
#include "catapult/model/BlockUtils.h"

namespace catapult { namespace fastfinality {

#define TEST_CLASS TimeSyncHandlersTests

	const auto SignerKeyPair = crypto::KeyPair::FromPrivate(test::GenerateRandomPrivateKey());

	auto createValidPrevoteMessagesRequest(const std::shared_ptr<model::Block>& pBlock) {
		auto pPacket = ionet::CreateSharedPacket<PushPrevoteMessagesRequest>(pBlock->Size);
		pPacket->MessageCount = 1;
		std::memcpy(static_cast<void*>(pPacket.get() + 1), pBlock.get(), pBlock->Size);

		return pPacket;
	}

	auto createValidRequest(const std::shared_ptr<model::Block>& pBlock, ionet::PacketType packetType) {
		auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(pBlock->Size);
		pPacket->Type = packetType;
		std::memcpy(static_cast<void*>(pPacket.get() + 1), pBlock.get(), pBlock->Size);

		return pPacket;
	}

	namespace {
		struct WeightedVotingServiceTraits {
			static constexpr auto CreateRegistrar = CreateWeightedVotingServiceRegistrar;
		};

		using TestContext = test::ServiceLocatorTestContext<WeightedVotingServiceTraits>;

		template<typename THandler, typename TAct>
		void RunPushHandlerTest(CommitteeRound& round, THandler handlerFn, TAct actFunc) {
			// Arrange:
			const auto keyPair = crypto::KeyPair::FromPrivate(test::GenerateRandomPrivateKey());
			std::shared_ptr<thread::IoThreadPool> pPool = test::CreateStartedIoThreadPool();
			auto config = test::CreateUninitializedBlockchainConfiguration();
			auto dbrbConfig = dbrb::DbrbConfiguration::Uninitialized();
			const_cast<utils::FileSize&>(config.Node.MaxPacketDataSize) = utils::FileSize::FromMegabytes(150);
			auto connectionSettings = extensions::GetConnectionSettings(config);

			TestContext context;
			auto& pluginManager = context.testState().pluginManager();
			pluginManager.setCommitteeManager(std::make_shared<mocks::MockCommitteeManager>(SignerKeyPair.publicKey()));
			pluginManager.setDbrbViewFetcher(std::make_shared<mocks::MockDbrbViewFetcher>());

			const auto& storage = context.testState().state().storage();
			pluginManager.getCommitteeManager().setLastBlockElementSupplier([&storage]() {
				auto storageView = storage.view();
				return storageView.loadBlockElement(storageView.chainHeight());
			});
			context.testState().state().hooks().setTransactionRangeConsumerFactory([](auto) { return [](auto&&) {}; });

			auto& state = context.testState().state();
			auto pWriters = net::CreatePacketWriters(pPool, keyPair, connectionSettings, state);
			dbrb::TransactionSender transactionSender(keyPair, config.Immutable, dbrbConfig, state.hooks().transactionRangeConsumerFactory()(disruptor::InputSource::Local));
			auto chainHeightSupplier = [&storage]() {
				return storage.view().chainHeight();
			};
			auto pDbrbProcess = std::make_shared<dbrb::DbrbProcess>(pWriters, state.packetIoPickers(), config::ToLocalDbrbNode(config),
																	keyPair, pPool, std::move(transactionSender), state.pluginManager().dbrbViewFetcher(), state.timeSupplier(), chainHeightSupplier);

			std::shared_ptr<WeightedVotingFsm> pFsm = std::make_shared<WeightedVotingFsm>(pPool, config, pDbrbProcess);
			pFsm->committeeData().setCommitteeRound(round);

			// Register a handler
			ionet::ServerPacketHandlers handlers;
			handlerFn(pFsm, handlers, context);

			// Act
			actFunc(pFsm, handlers, context);
		}
	}

	// region PushProposedBlock

	namespace {
		template<typename TModify, typename TAssert>
		void RunPushProposedBlockTest(CommitteeRound round, TModify modifyFunc, TAssert assertFunc) {
			RunPushHandlerTest(
					round,
					[](auto pFsm, auto& handlers, auto& context) {},
					[modifyFunc, assertFunc](auto pFsm, auto& handlers, auto& context) {
						// Arrange:
						auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
						auto& pluginManager = context.testState().pluginManager();
						auto& committee = pluginManager.getCommitteeManager().committee();
						pBlock->Signer = committee.BlockProposer;
						model::SignBlockHeader(SignerKeyPair, *pBlock);

						auto& committeeData = pFsm->committeeData();
						auto packetType = ionet::PacketType::Pull_Confirmed_Block;
						auto pPacket = createValidRequest(pBlock, packetType);

						modifyFunc(pBlock, pPacket, committeeData);

						// Act:
						PushProposedBlock(pFsm, pluginManager, *pPacket);

						// Assert:
                        assertFunc(pBlock, committeeData.proposedBlock());
					});
		}
	}

	TEST(TEST_CLASS, PushProposedBlock_Success) {
		// Arrange:
		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
		RunPushProposedBlockTest(
				CommitteeRound { 0u, CommitteePhase::Commit, utils::ToTimePoint(Timestamp()), 0u },
				[](auto pBlock, auto pPacket, auto& committeeData){},
                [](auto sentBlock, auto proposedBlock){
                    ASSERT_EQ(*proposedBlock, *sentBlock);
                }
		);
	}

	TEST(TEST_CLASS, PushProposedBlock_InvalidPacket) {
		// Arrange:
		RunPushProposedBlockTest(
				CommitteeRound { 0u, CommitteePhase::Commit, utils::ToTimePoint(Timestamp()), 0u },
				[](auto pBlock, auto pPacket, auto& committeeData){
					pPacket->Size /= 2;
				},
                [](auto sentBlock, auto proposedBlock){
                    ASSERT_TRUE(proposedBlock == nullptr);
                }
		);
	}

	TEST(TEST_CLASS, PushProposedBlock_ExistProposal) {
		// Arrange:
		RunPushProposedBlockTest(
				CommitteeRound { 0u, CommitteePhase::Commit, utils::ToTimePoint(Timestamp()), 0u },
				[](auto pBlock, auto pPacket, auto& committeeData){
					auto pFakeBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
					committeeData.setProposedBlock(pFakeBlock);
				},
                [](auto sentBlock, auto proposedBlock){
                    ASSERT_NE(*sentBlock, *proposedBlock);
                }
		);
	}

	TEST(TEST_CLASS, PushProposedBlock_BadSigner) {
		// Arrange:
		RunPushProposedBlockTest(
				CommitteeRound { 0u, CommitteePhase::Commit, utils::ToTimePoint(Timestamp()), 0u },
				[](auto pBlock, auto pPacket, auto& committeeData){
					pBlock->Signer = test::GenerateRandomByteArray<Key>();
					std::memcpy(static_cast<void*>(pPacket.get() + 1), pBlock.get(), pBlock->Size);
				},
                [](auto sentBlock, auto proposedBlock){
                    ASSERT_TRUE(proposedBlock == nullptr);
                }
		);
	}

	TEST(TEST_CLASS, PushProposedBlock_BadRound) {
		// Arrange:
		RunPushProposedBlockTest(
				CommitteeRound { 0u, CommitteePhase::Commit, utils::ToTimePoint(Timestamp()), 0u },
				[](auto pBlock, auto pPacket, auto& committeeData){
					pBlock->setRound(6);
					std::memcpy(static_cast<void*>(pPacket.get() + 1), pBlock.get(), pBlock->Size);
				},
                [](auto sentBlock, auto proposedBlock){
                    ASSERT_TRUE(proposedBlock == nullptr);
                }
		);
	}

	TEST(TEST_CLASS, PushProposedBlock_BlockHeaderSignature) {
		// Arrange:
		RunPushProposedBlockTest(
				CommitteeRound { 0u, CommitteePhase::Commit, utils::ToTimePoint(Timestamp()), 0u },
				[](auto pBlock, auto pPacket, auto& committeeData){
					pBlock->Signature = test::GenerateRandomByteArray<Signature>();
					std::memcpy(static_cast<void*>(pPacket.get() + 1), pBlock.get(), pBlock->Size);
				},
                [](auto sentBlock, auto proposedBlock){
                    ASSERT_TRUE(proposedBlock == nullptr);
                }
		);
	}

	// endregion

	// region RegisterPullConfirmedBlockHandler

	namespace {
		void RunPullConfirmedBlockHandlerTest(
				const std::shared_ptr<model::Block>& pBlock,
				CommitteeRound round,
				bool existBlock) {
			RunPushHandlerTest(
					round,
					[](auto pFsm, auto& handlers, auto& context) {
						RegisterPullConfirmedBlockHandler(pFsm, handlers);
					},
					[pBlock, existBlock](auto pFsm, auto& handlers, auto& _) {
						auto packetType = ionet::PacketType::Pull_Confirmed_Block;
						auto pPacket = createValidRequest(pBlock, packetType);

						// mock proposed block
						if (existBlock) {
							auto& committeeData = pFsm->committeeData();
							committeeData.setConfirmedBlock(pBlock);
						}

						// Act:
						ionet::ServerPacketHandlerContext context({}, "");
						EXPECT_TRUE(handlers.process(*pPacket, context));

						// Assert:
						ASSERT_TRUE(context.hasResponse());
						EXPECT_EQ(packetType, context.response().header().Type);
						std::vector<std::shared_ptr<model::Block>> response;
						auto pConfirmedBlock = pFsm->committeeData().confirmedBlock();
						if (pConfirmedBlock)
							response.push_back(pConfirmedBlock);
						auto expectedPayload = ionet::PacketPayloadFactory::FromEntities(packetType, response);
						test::AssertEqualPayload(expectedPayload, context.response());
					});
		}
	}

	TEST(TEST_CLASS, PullConfirmedBlockHandler_ExistConfirmedBlock) {
		// Arrange:
		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
		RunPullConfirmedBlockHandlerTest(
			pBlock,
			CommitteeRound { 0u, CommitteePhase::Commit, utils::ToTimePoint(Timestamp()), 0u },
			true
		);
	}

	TEST(TEST_CLASS, PullConfirmedBlockHandler_DoNotExistConfirmedBlock) {
		// Arrange:
		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
		RunPullConfirmedBlockHandlerTest(
			pBlock,
			CommitteeRound { 0u, CommitteePhase::Commit, utils::ToTimePoint(Timestamp()), 0u },
			false
		);
	}

	// endregion

	// region RegisterPushVoteMessagesHandler

	namespace {
		template<typename TArrange>
		void RunPushVoteMessagesHandlerTest(
				CommitteeRound round,
				ionet::PacketType packetType,
				bool existBlock,
				bool shouldHaveVote,
				TArrange arrange) {
			RunPushHandlerTest(
					round,
					[](auto pFsm, auto& handlers, auto& context) {},
					[existBlock, packetType, shouldHaveVote, arrange](auto pFsm, auto& handlers, auto& context) {
						// Arrange
						const crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(test::GenerateRandomPrivateKey());
						const crypto::KeyPair cosigKeyPair = crypto::KeyPair::FromPrivate(test::GenerateRandomPrivateKey());
						auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
						model::SignBlockHeader(keyPair, *pBlock);
						catapult::Signature blockCosignature;
						model::CosignBlockHeader(cosigKeyPair, *pBlock, blockCosignature);

						auto& committeeData = pFsm->committeeData();
						committeeData.setProposedBlock(pBlock);

						auto pPacket = createValidPrevoteMessagesRequest(pBlock);
						auto* pMessage = reinterpret_cast<CommitteeMessage*>(pPacket.get() + 1);
						pMessage->BlockHash = committeeData.proposedBlockHash();
						pMessage->BlockCosignature.Signer = cosigKeyPair.publicKey();
						pMessage->BlockCosignature.Signature = blockCosignature;
						crypto::Sign(cosigKeyPair, CommitteeMessageDataBuffer(*pMessage), pMessage->MessageSignature);
						const auto& signer = pMessage->BlockCosignature.Signer;

						arrange(committeeData, pMessage);

						// Act:
						PushPrevoteMessages(pFsm, *pPacket);

						// Assert:
						if (shouldHaveVote)
							ASSERT_TRUE(committeeData.hasVote(signer, pMessage->Type));
						else {
							ASSERT_TRUE(!committeeData.hasVote(signer, pMessage->Type));
						}
					});
		}
	}

	TEST(TEST_CLASS, PushVoteMessagesHandler_Success) {
		// Arrange:
		RunPushVoteMessagesHandlerTest(
				CommitteeRound { 0u, CommitteePhase::Commit, utils::ToTimePoint(Timestamp()), 0u },
				ionet::PacketType::Push_Prevote_Messages,
				true,
				true,
				[](auto& committeeData, auto* pMessage){}
		);
	}

	TEST(TEST_CLASS, PushVoteMessagesHandler_AlreadyHasVote) {
		// Arrange:
		RunPushVoteMessagesHandlerTest(
			CommitteeRound { 0u, CommitteePhase::Commit, utils::ToTimePoint(Timestamp()), 0u },
			ionet::PacketType::Push_Prevote_Messages,
			true,
			true,
			[](auto& committeeData, auto* pMessage){
				committeeData.addVote(*pMessage);
			}
		);
	}

	TEST(TEST_CLASS, PushVoteMessagesHandler_NoProposedBlock) {
		// Arrange:
		RunPushVoteMessagesHandlerTest(
			CommitteeRound { 0u, CommitteePhase::Commit, utils::ToTimePoint(Timestamp()), 0u },
			ionet::PacketType::Push_Prevote_Messages,
			true,
			false,
			[](auto& committeeData, auto* pMessage){
				committeeData.setProposedBlock(nullptr);
			}
		);
	}

	TEST(TEST_CLASS, PushVoteMessagesHandler_BadMessageBlockHash) {
		// Arrange:
		RunPushVoteMessagesHandlerTest(
			CommitteeRound { 0u, CommitteePhase::Commit, utils::ToTimePoint(Timestamp()), 0u },
			ionet::PacketType::Push_Prevote_Messages,
			true,
			false,
			[](auto& committeeData, auto* pMessage){
				auto pFakeBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
				committeeData.setProposedBlock(pFakeBlock);
			}
		);
	}

	TEST(TEST_CLASS, PushVoteMessagesHandler_BadMessageSignature) {
		// Arrange:
		RunPushVoteMessagesHandlerTest(
			CommitteeRound { 0u, CommitteePhase::Commit, utils::ToTimePoint(Timestamp()), 0u },
			ionet::PacketType::Push_Prevote_Messages,
			true,
			false,
			[](auto& committeeData, auto* pMessage){
					pMessage->BlockCosignature.Signer = test::GenerateRandomArray<Key_Size>();
			}
		);
	}

	// endregion
}}