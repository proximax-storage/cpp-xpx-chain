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
		struct CommitteeStage;
		class WeightedVotingFsm;
	}
	namespace extensions { class ServiceState; }
}

namespace catapult { namespace fastfinality {

	/// A retriever that returns remote node states from all available peers.
	using RemoteNodeStateRetriever = std::function<thread::future<std::vector<RemoteNodeState>> ()>;

	/// Handles a push proposed block message.
	consumer<const ionet::Packet&> PushProposedBlock(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const plugins::PluginManager& pluginManager);

	/// Handles a push confirmed block message.
	consumer<const ionet::Packet&> PushConfirmedBlock(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const plugins::PluginManager& pluginManager);

	/// Handles a push prevote message.
	consumer<const ionet::Packet&> PushPrevoteMessages(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak);

	/// Handles a push precommit message.
	consumer<const ionet::Packet&> PushPrecommitMessages(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak);

	/// Registers a pull remote node state handler in \a handlers constructing response from \a pFsmWeak
	/// using \a pConfigHolder, \a blockElementGetter and \a lastBlockElementSupplier.
	void RegisterPullRemoteNodeStateHandler(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		ionet::ServerPacketHandlers& handlers,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const std::function<std::shared_ptr<const model::BlockElement> (const Height&)>& blockElementGetter,
		const model::BlockElementSupplier& lastBlockElementSupplier);
}}