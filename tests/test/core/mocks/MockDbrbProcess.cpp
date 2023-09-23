/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MockDbrbProcess.h"
#include "src/catapult/crypto/Signer.h"

#include <utility>

namespace catapult { namespace mocks {

	MockDbrbProcess::MockDbrbProcess(
			const dbrb::ProcessId& processId,
			bool fakeDissemination,
			const dbrb::DbrbConfiguration& dbrbConfig,
			std::weak_ptr<net::PacketWriters> pWriters,
			const net::PacketIoPickerContainer& packetIoPickers,
			const crypto::KeyPair& keyPair,
			const config::ImmutableConfiguration& immutableConfig,
			const std::shared_ptr<thread::IoThreadPool>& pPool,
			const dbrb::DbrbViewFetcher& dbrbViewFetcher,
			chain::TimeSupplier timeSupplier,
			const supplier<Height>& chainHeightSupplier)
		: DbrbProcess(std::move(pWriters),
					  packetIoPickers,
					  { processId, {}, {} },
					  keyPair,
					  pPool,
					  { keyPair, immutableConfig, dbrbConfig, {}, {} },
					  dbrbViewFetcher,
					  std::move(timeSupplier),
					  chainHeightSupplier) {
		m_fakeDissemination = fakeDissemination;
		setDeliverCallback([this](const dbrb::Payload& payload) {
			const auto payloadHash = dbrb::CalculatePayloadHash(payload);
		  	this->m_deliveredPayloads.insert(payloadHash);
		});
	}

	void MockDbrbProcess::setCurrentView(const dbrb::View& view) {
		m_currentView = view;
	}

	void MockDbrbProcess::broadcast(const dbrb::Payload& payload) {
		CATAPULT_LOG(debug) << "[DBRB] BROADCAST: payload " << payload->Type;
		if (!m_currentView.isMember(m_id)) {
			CATAPULT_LOG(debug) << "[DBRB] BROADCAST: not a member of the current view " << m_currentView << ", aborting broadcast.";
			return;
		}

		CATAPULT_LOG(debug) << "[DBRB] BROADCAST: sending payload " << payload->Type;
		auto pMessage = std::make_shared<dbrb::PrepareMessage>(m_id, payload, m_currentView);
		disseminate(pMessage, pMessage->View.Data);
	}

	void MockDbrbProcess::processMessage(const dbrb::Message& message) {
		DbrbProcess::processMessage(message);
	}

	Signature MockDbrbProcess::sign(const dbrb::Payload& payload) {
		uint32_t packetPayloadSize = m_currentView.packedSize();
		auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(packetPayloadSize);
		auto pBuffer = pPacket->Data();
		Write(pBuffer, m_currentView);

		auto hash = dbrb::CalculateHash({ { reinterpret_cast<const uint8_t*>(payload.get()), payload->Size }, { pPacket->Data(), packetPayloadSize } });
		Signature signature;
		crypto::Sign(m_keyPair, hash, signature);

		return signature;
	}

	void MockDbrbProcess::disseminate(const std::shared_ptr<dbrb::Message>& pMessage, std::set<dbrb::ProcessId> recipients) {
		auto pPacket = pMessage->toNetworkPacket(&m_keyPair);
		m_disseminationHistory.emplace_back(pMessage, recipients);

		if (m_fakeDissemination)
			return;

		for (const auto& pProcess : DbrbProcessPool) {
			if (recipients.count(pProcess->m_id)) {
				pProcess->processMessage(*pMessage);
			}
		}
	}

	void MockDbrbProcess::send(const std::shared_ptr<dbrb::Message>& pMessage, const dbrb::ProcessId& recipient) {
		disseminate(pMessage, std::set<dbrb::ProcessId>{ recipient });
	}

	void MockDbrbProcess::onAcknowledgedMessageReceived(const dbrb::AcknowledgedMessage& message) {
		// Message sender must be a member of the view specified in the message.
		if (!message.View.isMember(message.Sender)) {
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Aborting message processing (sender is not in supplied view).";
			return;
		}

		auto& data = m_broadcastData[message.PayloadHash];
		if (!data.Payload) {
			CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Aborting message processing (no payload).";
			return;
		}

		CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: payload " << data.Payload->Type << " from " << data.Sender;

		data.Signatures[std::make_pair(message.View, message.Sender)] = message.PayloadSignature;
		bool quorumCollected = data.QuorumManager.update(message);
		if (quorumCollected && data.Certificate.empty())
			onAcknowledgedQuorumCollected(message);
	}

	void MockDbrbProcess::onAcknowledgedQuorumCollected(const dbrb::AcknowledgedMessage& message) {
		// Replacing certificate.
		auto& data = m_broadcastData[message.PayloadHash];
		CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Quorum collected in view " << message.View << ". Payload " << data.Payload->Type << " from " << data.Sender;
		data.CertificateView = message.View;
		data.Certificate.clear();
		const auto& acknowledgedSet = data.QuorumManager.AcknowledgedPayloads[message.View];
		for (const auto& [processId, hash] : acknowledgedSet) {
			if (hash == message.PayloadHash)
				data.Certificate[processId] = data.Signatures.at(std::make_pair(message.View, processId));
		}

		// Disseminating Commit message.
		CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: Disseminating Commit message with payload " << data.Payload->Type << " from " << data.Sender;
		auto pMessage = std::make_shared<dbrb::CommitMessage>(m_id, message.PayloadHash, data.Certificate, data.CertificateView, m_currentView);
		disseminate(pMessage, m_currentView.Data);
	}

	void MockDbrbProcess::onCommitMessageReceived(const dbrb::CommitMessage& message) {
		// View specified in the message must be equal to the current view of the process.
		if (message.CurrentView != m_currentView) {
			CATAPULT_LOG(debug) << "[DBRB] COMMIT: Aborting message processing (supplied view is not a current view).";
			return;
		}

		auto& data = m_broadcastData[message.PayloadHash];
		if (!data.Payload) {
			CATAPULT_LOG(debug) << "[DBRB] COMMIT: Aborting message processing (no payload).";
			return;
		}

		CATAPULT_LOG(debug) << "[DBRB] COMMIT: payload " << data.Payload->Type << " from " << data.Sender;

		// Update stored PayloadData and ProcessState, if necessary,
		// and disseminate Commit message with updated view.
		if (!data.CommitMessageReceived) {
			data.CommitMessageReceived = true;

			CATAPULT_LOG(debug) << "[DBRB] COMMIT: Disseminating Commit message with payload " << data.Payload->Type << " from " << data.Sender;
			auto pMessage = std::make_shared<dbrb::CommitMessage>(m_id, message.PayloadHash, message.Certificate, message.CertificateView, m_currentView);
			disseminate(pMessage, m_currentView.Data);
		}

		// Allow delivery for sender process.
		CATAPULT_LOG(debug) << "[DBRB] COMMIT: Sending Deliver message with payload " << data.Payload->Type << " from " << data.Sender << " to " << message.Sender;
		auto pMessage = std::make_shared<dbrb::DeliverMessage>(m_id, message.PayloadHash, m_currentView);
		send(pMessage, message.Sender);
	}

	const std::set<Hash256>& MockDbrbProcess::deliveredPayloads() {
		return m_deliveredPayloads;
	}

	const dbrb::ProcessId& MockDbrbProcess::id() {
		return m_id;
	}

	const dbrb::View& MockDbrbProcess::currentView() {
		return m_currentView;
	}

	std::map<Hash256, dbrb::BroadcastData>& MockDbrbProcess::broadcastData() {
		return m_broadcastData;
	}

	const MockDbrbProcess::DisseminationHistory& MockDbrbProcess::disseminationHistory() {
		return m_disseminationHistory;
	}

	dbrb::QuorumManager& MockDbrbProcess::getQuorumManager(const Hash256& payloadHash) {
		return m_broadcastData[payloadHash].QuorumManager;
	}
}}
