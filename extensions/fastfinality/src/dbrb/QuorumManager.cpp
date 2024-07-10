/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "QuorumManager.h"

namespace catapult { namespace dbrb {

	bool QuorumManager::update(const AcknowledgedMessage& message, ionet::PacketType payloadType) {
		CATAPULT_LOG(trace) << "[DBRB] QUORUM: Received ACKNOWLEDGED message in view " << message.View << ".";
		auto& set = AcknowledgedPayloads[message.View];
		const auto& payloadHash = message.PayloadHash;
		set.emplace(message.Sender, payloadHash);

		const auto acknowledgedCount = std::count_if(set.begin(), set.end(), [&payloadHash](const auto& pair) {
			return pair.second == payloadHash;
		});

		auto quorumSize = message.View.quorumSize();
		const auto triggered = (acknowledgedCount == quorumSize);
		CATAPULT_LOG(debug) << "[DBRB] QUORUM: ACKNOWLEDGE " << payloadType << " quorum status is " << acknowledgedCount << "/" << quorumSize << (triggered ? " (TRIGGERED)." : " (NOT triggered).");

		return triggered;
	}

	bool QuorumManager::update(const DeliverMessage& message, ionet::PacketType payloadType) {
		auto& set = DeliverQuorumCollectedProcesses[message.View];
		if (set.emplace(message.Sender).second) {
			auto quorumSize = message.View.quorumSize();
			bool triggered = set.size() == quorumSize;
			CATAPULT_LOG(debug) << "[DBRB] QUORUM: DELIVER " << payloadType << " quorum status is " << set.size() << "/" << quorumSize << (triggered ? " (TRIGGERED)." : " (NOT triggered).");
			return triggered;
		} else {
			return false;
		}
	}

	bool QuorumManager::update(const ConfirmDeliverMessage& message, const View& bootstrapView, ionet::PacketType payloadType) {
		auto& set = ConfirmedDeliverProcesses[message.View];
		if (set.emplace(message.Sender).second) {
			auto quorumSize = bootstrapView.quorumSize();
			bool triggered = set.size() == quorumSize;
			CATAPULT_LOG(debug) << "[DBRB] QUORUM: CONFIRM DELIVER " << payloadType << " quorum status is " << set.size() << "/" << quorumSize << (triggered ? " (TRIGGERED)." : " (NOT triggered).");
			return triggered;
		} else {
			return false;
		}
	}
}}