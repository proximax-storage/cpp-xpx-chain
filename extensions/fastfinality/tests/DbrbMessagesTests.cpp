/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "fastfinality/src/WeightedVotingFsm.h"
#include "catapult/crypto/KeyPair.h"
#include "tests/TestHarness.h"

namespace catapult { namespace fastfinality {

	namespace {
		dbrb::CommitMessage CreateCommitMessage(const std::vector<dbrb::ProcessId>& nodes) {
			dbrb::View view;
			view.Data = dbrb::ViewData{ nodes[0], nodes[2], nodes[4] };
			auto payload = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
			auto payloadHash = dbrb::CalculatePayloadHash(payload);
			std::map<dbrb::ProcessId, catapult::Signature> certificate{
				{ nodes[0], test::GenerateRandomArray<Signature_Size>() },
				{ nodes[1], test::GenerateRandomArray<Signature_Size>() },
				{ nodes[2], test::GenerateRandomArray<Signature_Size>() },
				{ nodes[3], test::GenerateRandomArray<Signature_Size>() },
			};
			return dbrb::CommitMessage(nodes[0], payloadHash, certificate, view);
		}

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
			auto keyPair = crypto::KeyPair::FromString("CBD84EF8F5F38A25C01308785EA99627DE897D151AFDFCDA7AB07EFD8ED98534");
			auto pPacket = originalMessage.toNetworkPacket(&keyPair);
			auto pUnpackedMessage = dbrb::NetworkPacketConverter().toMessage(*pPacket);
			const auto& unpackedMessage = reinterpret_cast<const TMessage&>(*pUnpackedMessage);
			EXPECT_EQ(originalMessage.Sender, unpackedMessage.Sender);
			EXPECT_EQ(originalMessage.Type, unpackedMessage.Type);
			EXPECT_EQ(originalMessage.Signature, unpackedMessage.Signature);

			callback(originalMessage, unpackedMessage);
		}
	}

	TEST(TEST_CLASS, ValidatePrepareMessageSerialization) {
		RunMessageSerializationTest<dbrb::PrepareMessage>([](const auto& nodes) {
				dbrb::View view;
				for (const auto& node : nodes)
					view.Data.emplace(node);
				auto payload = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
				return dbrb::PrepareMessage(nodes[0], payload, view);
			},
			[](const dbrb::PrepareMessage& originalMessage, const dbrb::PrepareMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.View, unpackedMessage.View);
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
				return dbrb::AcknowledgedMessage(nodes[0], payloadHash, view, test::GenerateRandomArray<Signature_Size>());
			},
			[](const dbrb::AcknowledgedMessage& originalMessage, const dbrb::AcknowledgedMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.View, unpackedMessage.View);
				EXPECT_EQ(originalMessage.PayloadHash, unpackedMessage.PayloadHash);
			});
	}

	TEST(TEST_CLASS, ValidateCommitMessageSerialization) {
		RunMessageSerializationTest<dbrb::CommitMessage>([](const auto& nodes) {
				return CreateCommitMessage(nodes);
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
}}
