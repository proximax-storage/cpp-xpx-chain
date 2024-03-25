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
		std::weak_ptr<net::PacketWriters> pWriters,
		const ionet::NodeContainer& nodeContainer,
		const crypto::KeyPair& keyPair,
		const std::shared_ptr<thread::IoThreadPool>& pPool,
		const dbrb::DbrbViewFetcher& dbrbViewFetcher,
		const dbrb::DbrbConfiguration& dbrbConfig)
			: DbrbProcess(
				ionet::Node{ processId, ionet::NodeEndpoint(), ionet::NodeMetadata() },
				keyPair,
				nodeContainer,
				std::move(pWriters),
				pPool,
				nullptr,
				dbrbViewFetcher,
				dbrbConfig) {
		m_fakeDissemination = fakeDissemination;
		setDeliverCallback([this](const dbrb::Payload& payload) {
			const auto payloadHash = dbrb::CalculatePayloadHash(payload);
		  	this->m_deliveredPayloads.insert(payloadHash);
		});
	}

	void MockDbrbProcess::setCurrentView(const dbrb::View& view) {
		m_currentView = view;
	}

	void MockDbrbProcess::broadcast(const dbrb::Payload& payload, std::set<dbrb::ProcessId> recipients) {
		dbrb::View broadcastView{ recipients };
		if (!(broadcastView <= m_currentView)) {
			CATAPULT_LOG(warning) << "[DBRB] BROADCAST: " << broadcastView << " is not a subview of the current view " << m_currentView << ", aborting broadcast";
			return;
		}

		CATAPULT_LOG(debug) << "[DBRB] BROADCAST: payload " << payload->Type;
		if (!broadcastView.isMember(m_id)) {
			CATAPULT_LOG(warning) << "[DBRB] BROADCAST: not a member of the current view " << broadcastView << ", aborting broadcast.";
			return;
		}

		auto payloadHash = dbrb::CalculatePayloadHash(payload);
		auto& data = m_broadcastData[payloadHash];
		data.Payload = payload;
		data.BroadcastView = broadcastView;

		CATAPULT_LOG(debug) << "[DBRB] BROADCAST: " << m_id << " is sending payload " << payload->Type;
		auto pMessage = std::make_shared<dbrb::PrepareMessage>(m_id, payload, broadcastView);
		disseminate(pMessage, pMessage->View.Data);
	}

	void MockDbrbProcess::processMessage(const dbrb::Message& message) {
		DbrbProcess::processMessage(message);
	}

	Signature MockDbrbProcess::sign(const dbrb::Payload& payload, const dbrb::View& view) {
		uint32_t packetPayloadSize = view.packedSize();
		auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(packetPayloadSize);
		auto pBuffer = pPacket->Data();
		Write(pBuffer, view);

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
			if (recipients.count(pProcess->m_id))
				pProcess->processMessage(*pMessage);
		}
	}

	void MockDbrbProcess::send(const std::shared_ptr<dbrb::Message>& pMessage, const dbrb::ProcessId& recipient) {
		disseminate(pMessage, std::set<dbrb::ProcessId>{ recipient });
	}

	void MockDbrbProcess::onAcknowledgedMessageReceived(const dbrb::AcknowledgedMessage& message) {
		// Message sender must be a member of the view specified in the message.
		if (!message.View.isMember(message.Sender)) {
			CATAPULT_LOG(warning) << "[DBRB] ACKNOWLEDGED: Aborting message processing (sender is not in supplied view).";
			return;
		}

		auto& data = m_broadcastData[message.PayloadHash];
		if (!data.Payload) {
			CATAPULT_LOG(warning) << "[DBRB] ACKNOWLEDGED: Aborting message processing (no payload).";
			return;
		}

		if (message.View != data.BroadcastView) {
			CATAPULT_LOG(warning) << "[DBRB] ACKNOWLEDGED: Aborting message processing (supplied view is not the broadcast view)";
			return;
		}

		CATAPULT_LOG(debug) << "[DBRB] " << m_id << " got ACKNOWLEDGED message from " << message.Sender;

		data.Signatures[std::make_pair(message.View, message.Sender)] = message.PayloadSignature;
		bool quorumCollected = data.QuorumManager.update(message, data.Payload->Type);
		if (quorumCollected && data.Certificate.empty())
			onAcknowledgedQuorumCollected(message);
	}

	void MockDbrbProcess::onAcknowledgedQuorumCollected(const dbrb::AcknowledgedMessage& message) {
		// Replacing certificate.
		auto& data = m_broadcastData[message.PayloadHash];
		CATAPULT_LOG(debug) << "[DBRB] ACKNOWLEDGED: " << m_id << " collected quorum";
		data.Certificate.clear();
		const auto& acknowledgedSet = data.QuorumManager.AcknowledgedPayloads[message.View];
		for (const auto& [processId, hash] : acknowledgedSet) {
			if (hash == message.PayloadHash)
				data.Certificate[processId] = data.Signatures.at(std::make_pair(message.View, processId));
		}

		// Disseminating Commit message.
		CATAPULT_LOG(debug) << "[DBRB] " << m_id << " is disseminating COMMIT message";
		auto pMessage = std::make_shared<dbrb::CommitMessage>(m_id, message.PayloadHash, data.Certificate, data.BroadcastView);
		data.CommitMessageReceived = true;
		disseminate(pMessage, message.View.Data);
	}

	void MockDbrbProcess::onCommitMessageReceived(const dbrb::CommitMessage& message) {
		auto& data = m_broadcastData[message.PayloadHash];
		if (!data.Payload) {
			CATAPULT_LOG(warning) << "[DBRB] COMMIT: Aborting message processing (no payload).";
			return;
		}

		// View specified in the message must be equal to the current view of the process.
		if (message.View != data.BroadcastView) {
			CATAPULT_LOG(warning) << "[DBRB] COMMIT: Aborting message processing (supplied view is not a current view).";
			return;
		}

		CATAPULT_LOG(debug) << "[DBRB] " << m_id << " got COMMIT message from " << message.Sender;

		// Update stored PayloadData and ProcessState, if necessary,
		// and disseminate Commit message with updated view.
		if (!data.CommitMessageReceived) {
			data.CommitMessageReceived = true;

			CATAPULT_LOG(debug) << "[DBRB] " << m_id << " is disseminating COMMIT message";
			auto pMessage = std::make_shared<dbrb::CommitMessage>(m_id, message.PayloadHash, message.Certificate, message.View);
			disseminate(pMessage, message.View.Data);
		}

		// Allow delivery for sender process.
		CATAPULT_LOG(debug) << "[DBRB] COMMIT: " << m_id << " is sending DELIVER message to " << message.Sender;
		auto pMessage = std::make_shared<dbrb::DeliverMessage>(m_id, message.PayloadHash, message.View);
		send(pMessage, message.Sender);
	}

	const std::set<Hash256>& MockDbrbProcess::deliveredPayloads() {
		return m_deliveredPayloads;
	}

	const dbrb::ProcessId& MockDbrbProcess::id() {
		return m_id;
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
