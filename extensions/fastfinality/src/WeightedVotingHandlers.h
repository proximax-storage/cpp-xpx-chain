/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "WeightedVotingChainPackets.h"
#include "catapult/extensions/ServerHooks.h"
#include "catapult/plugins/PluginManager.h"
#include "catapult/thread/Future.h"
#include "catapult/types.h"
#include <functional>
#include <memory>

namespace catapult {
	namespace ionet { class NodePacketIoPair; }
	namespace fastfinality {
		struct CommitteeRound;
		class WeightedVotingFsm;
	}
	namespace extensions { class ServiceState; }
}

namespace catapult { namespace fastfinality {

	/// A retriever that returns remote node states from all available peers.
	using RemoteNodeStateRetriever = std::function<std::vector<RemoteNodeState> ()>;

	/// Validates a push proposed block message.
	bool ValidateProposedBlock(
		const WeightedVotingFsm& fsm,
		const plugins::PluginManager& pluginManager,
		const ionet::Packet& packet,
		std::shared_ptr<model::Block>& pBlock);

	/// Handles a push proposed block message.
	void PushProposedBlock(
		WeightedVotingFsm& fsm,
		const plugins::PluginManager& pluginManager,
		const ionet::Packet& packet);

	/// Validates a push prevote message.
	bool ValidatePrevoteMessages(
		WeightedVotingFsm& fsm,
		const ionet::Packet& packet);

	/// Handles a push prevote message.
	void PushPrevoteMessages(
		WeightedVotingFsm& fsm,
		const ionet::Packet& packet);

	/// Validates a push precommit message.
	bool ValidatePrecommitMessages(
		WeightedVotingFsm& fsm,
		const ionet::Packet& packet);

	/// Handles a push precommit message.
	void PushPrecommitMessages(
		WeightedVotingFsm& fsm,
		const ionet::Packet& packet);

	/// Registers a pull confirmed block handler in \a handlers constructing response from \a pFsmWeak.
	void RegisterPullConfirmedBlockHandler(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		ionet::ServerPacketHandlers& handlers);

	/// Registers a pull remote node state handler in \a handlers constructing response from \a pFsmWeak
	/// using \a pConfigHolder, \a blockElementGetter and \a lastBlockElementSupplier.
	void RegisterPullRemoteNodeStateHandler(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		ionet::ServerPacketHandlers& handlers,
		const Key& bootPublicKey,
		const std::function<std::shared_ptr<const model::BlockElement> (const Height&)>& blockElementGetter,
		const model::BlockElementSupplier& lastBlockElementSupplier);
}}