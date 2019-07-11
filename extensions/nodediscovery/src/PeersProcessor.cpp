/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "PeersProcessor.h"
#include "NodePingUtils.h"
#include "catapult/ionet/NodeContainer.h"

namespace catapult { namespace nodediscovery {

	PeersProcessor::PeersProcessor(
			const ionet::NodeContainer& nodeContainer,
			const NodePingRequestInitiator& pingRequestInitiator,
			extensions::ServiceState& state,
			const NodeConsumer& newPartnerNodeConsumer)
			: m_nodeContainer(nodeContainer)
			, m_pingRequestInitiator(pingRequestInitiator)
			, m_state(state)
			, m_newPartnerNodeConsumer(newPartnerNodeConsumer)
	{}

	void PeersProcessor::process(const ionet::NodeSet& candidateNodes) const {
		for (const auto& candidateNode : SelectUnknownNodes(m_nodeContainer.view(), candidateNodes)) {
			CATAPULT_LOG(debug) << "initiating ping with " << candidateNode;
			process(candidateNode);
		}
	}

	void PeersProcessor::process(const ionet::Node& candidateNode) const {
		auto networkIdentifier = m_state.networkIdentifier();
		auto newPartnerNodeConsumer = m_newPartnerNodeConsumer;
		m_pingRequestInitiator(candidateNode, [candidateNode, networkIdentifier, newPartnerNodeConsumer](
				auto result,
				const auto& responseNode) {
			CATAPULT_LOG(info) << "ping with " << candidateNode << " completed with: " << result;
			if (net::NodeRequestResult::Success != result)
				return;

			if (!IsNodeCompatible(responseNode, networkIdentifier, candidateNode.identityKey())) {
				CATAPULT_LOG(warning) << "ping with " << candidateNode << " rejected due to incompatibility";
				return;
			}

			// if the node responds without a host, use the endpoint that was used to ping it
			if (responseNode.endpoint().Host.empty())
				newPartnerNodeConsumer(ionet::Node(responseNode.identityKey(), candidateNode.endpoint(), responseNode.metadata()));
			else
				newPartnerNodeConsumer(responseNode);
		});
	}
}}
