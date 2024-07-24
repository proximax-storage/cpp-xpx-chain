/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "fastfinality/src/weighted_voting/WeightedVotingFsm.h"
#include "catapult/crypto/KeyPair.h"
#include "tests/TestHarness.h"

namespace catapult { namespace fastfinality {

	namespace {
		template<typename TMessage>
		void RunMessageSerializationTest(std::function<TMessage (const std::vector<dbrb::ProcessId>& nodes)> createMessage, consumer<const TMessage&, const TMessage&> callback) {
			std::vector<dbrb::ProcessId> nodes = {
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomByteArray<Key>(),
			};

			auto originalMessage = createMessage(nodes);
			auto pPacket = originalMessage.toNetworkPacket();
			auto pUnpackedMessage = dbrb::NetworkPacketConverter().toMessage(*pPacket);
			const auto& unpackedMessage = reinterpret_cast<const TMessage&>(*pUnpackedMessage);
			EXPECT_EQ(originalMessage.Sender, unpackedMessage.Sender);
			EXPECT_EQ(originalMessage.Type, unpackedMessage.Type);

			callback(originalMessage, unpackedMessage);
		}
	}

	TEST(TEST_CLASS, ValidatePrepareMessageSerialization) {
		RunMessageSerializationTest<dbrb::PrepareMessage>([](const auto& nodes) {
				dbrb::View view;
				for (const auto& node : nodes)
					view.Data.emplace(node);
				dbrb::View bootstrapView;
				for (auto i = 0u; i < nodes.size() / 2; ++i)
					bootstrapView.Data.emplace(nodes[i]);
				auto payload = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
				return dbrb::PrepareMessage(nodes[0], payload, view, bootstrapView);
			},
			[](const dbrb::PrepareMessage& originalMessage, const dbrb::PrepareMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.View, unpackedMessage.View);
				EXPECT_EQ(originalMessage.BootstrapView, unpackedMessage.BootstrapView);
				EXPECT_EQ(originalMessage.Payload->Size, unpackedMessage.Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.Payload.get(), unpackedMessage.Payload.get(), originalMessage.Payload->Size);
			});
	}

	TEST(TEST_CLASS, ValidateAcknowledgedMessageSerialization) {
		RunMessageSerializationTest<dbrb::AcknowledgedMessage>([](const auto& nodes) {
				dbrb::View view;
				for (const auto& node : nodes)
					view.Data.emplace(node);
				auto payload = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
				auto payloadHash = dbrb::CalculatePayloadHash(payload);
				return dbrb::AcknowledgedMessage(nodes[0], payloadHash, view, test::GenerateRandomByteArray<Signature>());
			},
			[](const dbrb::AcknowledgedMessage& originalMessage, const dbrb::AcknowledgedMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.View, unpackedMessage.View);
				EXPECT_EQ(originalMessage.PayloadHash, unpackedMessage.PayloadHash);
			});
	}

	TEST(TEST_CLASS, ValidateCommitMessageSerialization) {
		RunMessageSerializationTest<dbrb::CommitMessage>([](const auto& nodes) {
				dbrb::View view;
				view.Data = dbrb::ViewData{ nodes[0], nodes[2], nodes[4] };
				auto payload = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
				auto payloadHash = dbrb::CalculatePayloadHash(payload);
				std::map<dbrb::ProcessId, catapult::Signature> certificate{
					{ nodes[0], test::GenerateRandomByteArray<Signature>() },
					{ nodes[1], test::GenerateRandomByteArray<Signature>() },
					{ nodes[2], test::GenerateRandomByteArray<Signature>() },
					{ nodes[3], test::GenerateRandomByteArray<Signature>() },
				};
				return dbrb::CommitMessage(nodes[0], payloadHash, certificate, view);
			},
			[](const dbrb::CommitMessage& originalMessage, const dbrb::CommitMessage& unpackedMessage) {
			 	EXPECT_EQ(originalMessage.PayloadHash, unpackedMessage.PayloadHash);
				EXPECT_EQ(originalMessage.View, unpackedMessage.View);
			});
	}

	TEST(TEST_CLASS, ValidateDeliverMessageSerialization) {
		RunMessageSerializationTest<dbrb::DeliverMessage>([](const auto& nodes) {
				dbrb::View view;
				for (const auto& node : nodes)
					view.Data.emplace(node);
				auto payload = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
				auto payloadHash = dbrb::CalculatePayloadHash(payload);
				return dbrb::DeliverMessage(nodes[0], payloadHash, view);
			},
			[](const dbrb::DeliverMessage& originalMessage, const dbrb::DeliverMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.View, unpackedMessage.View);
				EXPECT_EQ(originalMessage.PayloadHash, unpackedMessage.PayloadHash);
			});
	}

	TEST(TEST_CLASS, ValidateShardPrepareMessageSerialization) {
		RunMessageSerializationTest<dbrb::ShardPrepareMessage>([](const auto& nodes) {
				dbrb::DbrbTreeView view;
				for (const auto& node : nodes)
					view.emplace_back(node);
				auto payload = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
				return dbrb::ShardPrepareMessage(nodes[0], payload, view, test::GenerateRandomByteArray<Signature>());
			},
			[](const dbrb::ShardPrepareMessage& originalMessage, const dbrb::ShardPrepareMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.Payload->Size, unpackedMessage.Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.Payload.get(), unpackedMessage.Payload.get(), originalMessage.Payload->Size);
				EXPECT_EQ(originalMessage.View, unpackedMessage.View);
				EXPECT_EQ(originalMessage.BroadcasterSignature, unpackedMessage.BroadcasterSignature);
			});
	}

	namespace {
		template<typename TMessage>
		void RunShardMessageSerializationTest() {
			RunMessageSerializationTest<TMessage>([](const auto& nodes) {
					auto payload = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
					auto payloadHash = dbrb::CalculatePayloadHash(payload);
					std::map<dbrb::ProcessId, catapult::Signature> certificate{
						{ nodes[0], test::GenerateRandomByteArray<Signature>() },
						{ nodes[1], test::GenerateRandomByteArray<Signature>() },
						{ nodes[2], test::GenerateRandomByteArray<Signature>() },
						{ nodes[3], test::GenerateRandomByteArray<Signature>() },
						{ nodes[4], test::GenerateRandomByteArray<Signature>() },
						{ nodes[5], test::GenerateRandomByteArray<Signature>() },
					};
					return TMessage(nodes[0], payloadHash, certificate);
				},
				[](const TMessage& originalMessage, const TMessage& unpackedMessage) {
					EXPECT_EQ(originalMessage.PayloadHash, unpackedMessage.PayloadHash);
					EXPECT_EQ(originalMessage.Certificate, unpackedMessage.Certificate);
				});
		}
	}

	TEST(TEST_CLASS, ValidateShardAcknowledgedMessage) {
		RunShardMessageSerializationTest<dbrb::ShardAcknowledgedMessage>();
	}

	TEST(TEST_CLASS, ValidateShardCommitMessage) {
		RunShardMessageSerializationTest<dbrb::ShardCommitMessage>();
	}

	TEST(TEST_CLASS, ValidateShardDeliverMessage) {
		RunShardMessageSerializationTest<dbrb::ShardDeliverMessage>();
	}
}}
