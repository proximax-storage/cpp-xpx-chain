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
		std::vector<std::shared_ptr<MockDbrbProcess>>& dbrbProcessPool,
		bool fakeDissemination,
		const ionet::NodeContainer& nodeContainer,
		const crypto::KeyPair& keyPair,
		const std::shared_ptr<thread::IoThreadPool>& pPool,
		const dbrb::DbrbViewFetcher& dbrbViewFetcher,
		const dbrb::DbrbConfiguration& dbrbConfig)
			: DbrbProcess(
				keyPair,
				dbrb::CreateMessageSender(ionet::Node{
					keyPair.publicKey(),
					ionet::NodeEndpoint(),
					ionet::NodeMetadata() },
					nodeContainer,
					dbrbConfig.IsDbrbProcess,
					pPool,
					utils::TimeSpan::FromMilliseconds(500)),
				pPool,
				nullptr,
				dbrbViewFetcher),
			DbrbProcessPool(dbrbProcessPool) {
		m_fakeDissemination = fakeDissemination;
		setDeliverCallback([this](const dbrb::Payload& payload) {
			const auto payloadHash = dbrb::CalculatePayloadHash(payload);
		  	this->m_deliveredPayloads.insert(payloadHash);
		});
		setGetDbrbModeCallback([]() { return dbrb::DbrbMode::Running; });
	}

	void MockDbrbProcess::setCurrentView(const dbrb::View& view) {
		m_currentView = view;
	}

	void MockDbrbProcess::setBootstrapView(const dbrb::View& view) {
		m_bootstrapView = view;
	}

	const dbrb::View& MockDbrbProcess::currentView() const {
		return m_currentView;
	}

	const dbrb::View& MockDbrbProcess::bootstrapView() const {
		return m_bootstrapView;
	}

	void MockDbrbProcess::broadcast(const dbrb::Payload& payload, std::set<dbrb::ProcessId> recipients) {
		dbrb::View broadcastView{ recipients };
		if (!(broadcastView <= m_currentView)) {
			CATAPULT_LOG(warning) << "[DBRB] BROADCAST: broadcast view is not a subview of the current view, aborting broadcast";
			return;
		}

		CATAPULT_LOG(debug) << "[DBRB] BROADCAST: payload " << payload->Type;
		if (!broadcastView.isMember(m_id)) {
			CATAPULT_LOG(warning) << "[DBRB] BROADCAST: not a member of the current view, aborting broadcast.";
			return;
		}

		auto payloadHash = dbrb::CalculatePayloadHash(payload);
		auto& data = m_broadcastData[payloadHash];
		data.Payload = payload;
		data.BroadcastView = broadcastView;
		data.BootstrapView = m_bootstrapView;

		CATAPULT_LOG(debug) << "[DBRB] BROADCAST: " << m_id << " is sending payload " << payload->Type;
		auto pMessage = std::make_shared<dbrb::PrepareMessage>(m_id, payload, broadcastView, m_bootstrapView);
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
		auto pPacket = pMessage->toNetworkPacket();
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
			onAcknowledgedQuorumCollected(message, data);
	}

	void MockDbrbProcess::onAcknowledgedQuorumCollected(const dbrb::AcknowledgedMessage& message, dbrb::BroadcastData& data) {
		// Replacing certificate.
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
		data.CommitMessageDisseminated = true;
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
		if (!data.CommitMessageDisseminated) {
			data.CommitMessageDisseminated = true;

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
