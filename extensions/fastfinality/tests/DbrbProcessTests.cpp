/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "fastfinality/src/WeightedVotingFsm.h"
#include "src/catapult/ionet/Packet.h"
#include "tests/test/core/mocks/MockDbrbProcess.h"
#include "tests/test/core/mocks/MockDbrbViewFetcher.h"
#include "tests/TestHarness.h"

using catapult::mocks::MockDbrbProcess;

namespace catapult { namespace fastfinality {

#define TEST_CLASS DbrbProcessTests

	namespace {
		void CreateMockDbrbProcesses(uint8_t count = 1, bool fakeDissemination = false) {
			dbrb::ViewData viewData;
			for (uint8_t i = 0u; i < count; ++i) {
				const auto processId = Key{ {i} };
				auto pProcess = std::make_shared<MockDbrbProcess>(processId, fakeDissemination);
				MockDbrbProcess::DbrbProcessPool.emplace_back(std::move(pProcess));
				viewData.insert(processId);
			}

			const dbrb::View currentView{ viewData };
			for (auto& pProcess : MockDbrbProcess::DbrbProcessPool)
				pProcess->setCurrentView(currentView);
		}

		std::shared_ptr<ionet::Packet> CreatePayload(
				uint32_t payloadSize = 1,
				ionet::PacketType packetType = ionet::PacketType::Undefined) {
			auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(payloadSize);
			pPacket->Type = packetType;
			return pPacket;
		}

		void AssertDeliveredPayloads(
				const std::shared_ptr<MockDbrbProcess>& pProcess,
				const std::set<Hash256>& expectedPayloads) {
			ASSERT_EQ(pProcess->deliveredPayloads(), expectedPayloads);
		}

		template<typename TMessage, typename... Args>
		std::shared_ptr<TMessage> CreateMessage(Args&&... args) {
			return std::make_shared<TMessage>(std::forward<Args>(args)...);
		}
	}

	TEST(TEST_CLASS, PayloadDeliverySuccess) {
		// Arrange:
		CreateMockDbrbProcesses(3);
		const auto pBroadcaster = MockDbrbProcess::DbrbProcessPool.front();

		const auto payload = CreatePayload();
		const auto payloadHash = dbrb::CalculatePayloadHash(payload);

		// Act:
		pBroadcaster->broadcast(payload);

		// Assert:
		const std::set<Hash256> expectedPayloads{ payloadHash };
		for (const auto& pProcess : MockDbrbProcess::DbrbProcessPool)
			AssertDeliveredPayloads(pProcess, expectedPayloads);
	}

	// region PrepareMessage

	TEST(TEST_CLASS, PrepareMessageReceivererIsNotParticipant) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		auto receiverView = pReceiver->currentView();
		receiverView.Data.erase(pReceiver->id());
		pReceiver->setCurrentView(receiverView); // Removing Receiver from its stored current view.
		const auto pMessage = CreateMessage<dbrb::PrepareMessage>(
				pSender->id(),
				CreatePayload(),
				pSender->currentView());

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		ASSERT_TRUE(pReceiver->disseminationHistory().empty());
	}

	TEST(TEST_CLASS, PrepareMessageSenderIsNotInSuppliedView) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		auto suppliedView = pSender->currentView();
		suppliedView.Data.erase(pSender->id());	// Removing Sender from the supplied view.
		const auto pMessage = CreateMessage<dbrb::PrepareMessage>(
				pSender->id(),
				CreatePayload(),
				suppliedView);

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		ASSERT_TRUE(pReceiver->disseminationHistory().empty());
	}

	TEST(TEST_CLASS, PrepareMessageSuppliedViewIsNotCurrent) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		auto suppliedView = pSender->currentView();
		const auto randomProcessId = test::GenerateRandomByteArray<dbrb::ProcessId>();
		suppliedView.Data.emplace(randomProcessId);	// Adding a random ProcessId to the supplied view.
		const auto pMessage = CreateMessage<dbrb::PrepareMessage>(
				pSender->id(),
				CreatePayload(),
				suppliedView);

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		ASSERT_TRUE(pReceiver->disseminationHistory().empty());
	}

	TEST(TEST_CLASS, PrepareMessageDuplicatePayloadHash) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		const auto payload = CreatePayload();
		const auto payloadHash = dbrb::CalculatePayloadHash(payload);
		auto& data = pReceiver->broadcastData()[payloadHash];	// Creating duplicate entry in broadcastData.
		data.Payload = payload;

		const auto pMessage = CreateMessage<dbrb::PrepareMessage>(
				pSender->id(),
				payload,
				pSender->currentView());

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		ASSERT_TRUE(pReceiver->disseminationHistory().empty());
	}

	TEST(TEST_CLASS, PrepareMessageSuccess) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		const auto pMessage = CreateMessage<dbrb::PrepareMessage>(
				pSender->id(),
				CreatePayload(),
				pSender->currentView());

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		ASSERT_EQ(pReceiver->disseminationHistory().size(), 1u);
	}

	// endregion

	// region AcknowledgedMessage

	TEST(TEST_CLASS, AcknowledgedMessageSenderIsNotInSuppliedView) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		const auto payload = CreatePayload();
		const auto payloadHash = dbrb::CalculatePayloadHash(payload);
		const auto payloadSignature = pSender->sign(payload);

		auto suppliedView = pSender->currentView();
		suppliedView.Data.erase(pSender->id());	// Removing Sender from the supplied view.
		const auto pMessage = CreateMessage<dbrb::AcknowledgedMessage>(
				pSender->id(),
				payloadHash,
				suppliedView,
				payloadSignature);

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		const auto& acknowledgedPayloads = pReceiver->getQuorumManager(payloadHash).AcknowledgedPayloads;
		ASSERT_TRUE(acknowledgedPayloads.empty());
	}

	TEST(TEST_CLASS, AcknowledgedMessageNoPayload) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		const auto payload = CreatePayload();
		const auto payloadHash = dbrb::CalculatePayloadHash(payload);
		const auto payloadSignature = pSender->sign(payload);

		// Not adding an entry in broadcastData.
		const auto pMessage = CreateMessage<dbrb::AcknowledgedMessage>(
				pSender->id(),
				payloadHash,
				pSender->currentView(),
				payloadSignature);

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		const auto& acknowledgedPayloads = pReceiver->getQuorumManager(payloadHash).AcknowledgedPayloads;
		ASSERT_TRUE(acknowledgedPayloads.empty());
	}

	TEST(TEST_CLASS, AcknowledgedMessageSuccess) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		const auto payload = CreatePayload();
		const auto payloadHash = dbrb::CalculatePayloadHash(payload);
		const auto payloadSignature = pSender->sign(payload);
		auto& data = pReceiver->broadcastData()[payloadHash];	// Creating correct entry in broadcastData.
		data.Payload = payload;

		const auto pMessage = CreateMessage<dbrb::AcknowledgedMessage>(
				pSender->id(),
				payloadHash,
				pSender->currentView(),
				payloadSignature);

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		const auto& acknowledgedPayloads = pReceiver->getQuorumManager(payloadHash).AcknowledgedPayloads;
		const auto& currentView = pReceiver->currentView();
		ASSERT_EQ(acknowledgedPayloads.count(currentView), 1u);
	}

	TEST(TEST_CLASS, AcknowledgedMessageSuccessWithQuorumCollected) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		const auto payload = CreatePayload();
		const auto payloadHash = dbrb::CalculatePayloadHash(payload);
		const auto payloadSignature = pSender->sign(payload);
		auto& data = pReceiver->broadcastData()[payloadHash];	// Creating correct entry in broadcastData.
		data.Payload = payload;

		// Adding Receiver's signature to the entry in broadcastData.
		const auto& currentView = pReceiver->currentView();
		data.Signatures[std::make_pair(currentView, pReceiver->id())] = pReceiver->sign(payload);

		// Adding Receiver to his list of AcknowledgedPayloads.
		auto& acknowledgedPayloads = pReceiver->getQuorumManager(payloadHash).AcknowledgedPayloads;
		acknowledgedPayloads[currentView].emplace(pReceiver->id(), payloadHash);

		const auto pMessage = CreateMessage<dbrb::AcknowledgedMessage>(
				pSender->id(),
				payloadHash,
				pSender->currentView(),
				payloadSignature);

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		ASSERT_EQ(acknowledgedPayloads[currentView].size(), 2u);	// Sender's message is added to AcknowledgedPayloads.
		ASSERT_EQ(pReceiver->disseminationHistory().size(), 1u);	// Commit message is disseminated to all processes.
	}

	// endregion

	// region CommitMessage

	TEST(TEST_CLASS, CommitMessageSuppliedViewIsNotCurrent) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		const auto payload = CreatePayload();
		const auto payloadHash = dbrb::CalculatePayloadHash(payload);

		auto suppliedView = pSender->currentView();
		const auto randomProcessId = test::GenerateRandomByteArray<dbrb::ProcessId>();
		suppliedView.Data.emplace(randomProcessId);	// Adding a random ProcessId to the supplied view.
		const auto pMessage = CreateMessage<dbrb::CommitMessage>(
				pSender->id(),
				payloadHash,
				dbrb::CertificateType(),
				dbrb::View(),
				suppliedView);

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		ASSERT_TRUE(pReceiver->disseminationHistory().empty());
	}

	TEST(TEST_CLASS, CommitMessageNoPayload) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		const auto payload = CreatePayload();
		const auto payloadHash = dbrb::CalculatePayloadHash(payload);

		// Not adding an entry in broadcastData.
		const auto pMessage = CreateMessage<dbrb::CommitMessage>(
				pSender->id(),
				payloadHash,
				dbrb::CertificateType(),
				dbrb::View(),
				pSender->currentView());

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		ASSERT_TRUE(pReceiver->disseminationHistory().empty());
	}

	TEST(TEST_CLASS, CommitMessageSuccess) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		const auto payload = CreatePayload();
		const auto payloadHash = dbrb::CalculatePayloadHash(payload);
		auto& data = pReceiver->broadcastData()[payloadHash];	// Creating correct entry in broadcastData.
		data.Payload = payload;

		const auto pMessage = CreateMessage<dbrb::CommitMessage>(
				pSender->id(),
				payloadHash,
				dbrb::CertificateType(),
				dbrb::View(),
				pSender->currentView());

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		// Disseminating Commit message to all processes and sending one Deliver message back to the Sender.
		ASSERT_EQ(pReceiver->disseminationHistory().size(), 2u);
	}

	TEST(TEST_CLASS, CommitMessageSuccessWithCommitReceived) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		const auto payload = CreatePayload();
		const auto payloadHash = dbrb::CalculatePayloadHash(payload);
		auto& data = pReceiver->broadcastData()[payloadHash];	// Creating correct entry in broadcastData.
		data.Payload = payload;

		data.CommitMessageReceived = true;

		const auto pMessage = CreateMessage<dbrb::CommitMessage>(
				pSender->id(),
				payloadHash,
				dbrb::CertificateType(),
				dbrb::View(),
				pSender->currentView());

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		// Only sending one Deliver message back to the Sender.
		ASSERT_EQ(pReceiver->disseminationHistory().size(), 1u);
	}

	// endregion

	// region DeliverMessage

	TEST(TEST_CLASS, DeliverMessageReceivererIsNotParticipant) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		const auto payload = CreatePayload();
		const auto payloadHash = dbrb::CalculatePayloadHash(payload);

		auto receiverView = pReceiver->currentView();
		receiverView.Data.erase(pReceiver->id());
		pReceiver->setCurrentView(receiverView); // Removing Receiver from its stored current view.
		const auto pMessage = CreateMessage<dbrb::DeliverMessage>(
				pSender->id(),
				payloadHash,
				pSender->currentView());

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		const auto& deliveredProcesses = pReceiver->getQuorumManager(payloadHash).DeliveredProcesses;
		ASSERT_TRUE(deliveredProcesses.empty());
	}

	TEST(TEST_CLASS, DeliverMessageSenderIsNotInSuppliedView) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		const auto payload = CreatePayload();
		const auto payloadHash = dbrb::CalculatePayloadHash(payload);

		auto suppliedView = pSender->currentView();
		suppliedView.Data.erase(pSender->id());	// Removing Sender from the supplied view.
		const auto pMessage = CreateMessage<dbrb::DeliverMessage>(
				pSender->id(),
				payloadHash,
				suppliedView);

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		const auto& deliveredProcesses = pReceiver->getQuorumManager(payloadHash).DeliveredProcesses;
		ASSERT_TRUE(deliveredProcesses.empty());
	}

	TEST(TEST_CLASS, DeliverMessageSenderNoPayload) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		const auto payload = CreatePayload();
		const auto payloadHash = dbrb::CalculatePayloadHash(payload);

		// Not adding an entry in broadcastData.
		const auto pMessage = CreateMessage<dbrb::DeliverMessage>(
				pSender->id(),
				payloadHash,
				pSender->currentView());

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		const auto& deliveredProcesses = pReceiver->getQuorumManager(payloadHash).DeliveredProcesses;
		ASSERT_TRUE(deliveredProcesses.empty());
	}

	TEST(TEST_CLASS, DeliverMessageSenderSuccess) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		const auto payload = CreatePayload();
		const auto payloadHash = dbrb::CalculatePayloadHash(payload);
		auto& data = pReceiver->broadcastData()[payloadHash];	// Creating correct entry in broadcastData.
		data.Payload = payload;

		const auto pMessage = CreateMessage<dbrb::DeliverMessage>(
				pSender->id(),
				payloadHash,
				pSender->currentView());

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		const auto& deliveredProcesses = pReceiver->getQuorumManager(payloadHash).DeliveredProcesses;
		const auto& currentView = pReceiver->currentView();
		ASSERT_EQ(deliveredProcesses.count(currentView), 1u);
	}

	TEST(TEST_CLASS, DeliverMessageSenderSuccessWithQuorumCollected) {
		// Arrange:
		CreateMockDbrbProcesses(2, true);
		const auto pSender = MockDbrbProcess::DbrbProcessPool.front();
		const auto pReceiver = MockDbrbProcess::DbrbProcessPool.back();

		const auto payload = CreatePayload();
		const auto payloadHash = dbrb::CalculatePayloadHash(payload);
		auto& data = pReceiver->broadcastData()[payloadHash];	// Creating correct entry in broadcastData.
		data.Payload = payload;

		// Adding Receiver to his list of DeliveredProcesses.
		const auto& currentView = pReceiver->currentView();
		auto& deliveredProcesses = pReceiver->getQuorumManager(payloadHash).DeliveredProcesses;
		deliveredProcesses[currentView].emplace(pReceiver->id());

		const auto pMessage = CreateMessage<dbrb::DeliverMessage>(
				pSender->id(),
				payloadHash,
				pSender->currentView());

		// Act:
		pReceiver->processMessage(*pMessage);

		// Assert:
		ASSERT_EQ(deliveredProcesses[currentView].size(), 2u);	// Sender's message is added to DeliveredProcesses.
		ASSERT_EQ(pReceiver->deliveredPayloads().count(payloadHash), 1u);	// Payload is delivered.
	}

	// endregion
}}
