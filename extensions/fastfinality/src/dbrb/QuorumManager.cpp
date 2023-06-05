/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "QuorumManager.h"

namespace catapult { namespace dbrb {

	bool QuorumManager::update(const AcknowledgedMessage& message) {
		CATAPULT_LOG(debug) << "[DBRB] QUORUM: Received ACKNOWLEDGED message in view " << message.View << ".";
		auto& set = AcknowledgedPayloads[message.View];
		const auto& payloadHash = message.PayloadHash;
		set.emplace(message.Sender, payloadHash);

		const auto acknowledgedCount = std::count_if(set.begin(), set.end(), [&payloadHash](const auto& pair) {
			return pair.second == payloadHash;
		});

		const auto triggered = (acknowledgedCount == message.View.quorumSize());
		CATAPULT_LOG(debug) << "[DBRB] QUORUM: ACK quorum status is " << acknowledgedCount << "/"
							<< message.View.quorumSize() << (triggered ? " (TRIGGERED)." : " (NOT triggered).");

		return triggered;
	}

	bool QuorumManager::update(const DeliverMessage& message) {
		auto& set = DeliveredProcesses[message.View];
		if (set.emplace(message.Sender).second) {
			bool triggered = set.size() == message.View.quorumSize();
			CATAPULT_LOG(debug) << "[DBRB] QUORUM: DELIVER quorum status is " << set.size() << "/"
								<< message.View.quorumSize()
								<< (triggered ? " (TRIGGERED)." : " (NOT triggered).");
			return triggered;
		} else {
			return false;
		}
	}
}}