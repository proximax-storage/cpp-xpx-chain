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

	/// Registers a push proposed block handler in \a handlers verifying data with \a pFsmWeak and \a pluginManager.
	void RegisterPushProposedBlockHandler(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		ionet::ServerPacketHandlers& handlers,
		const plugins::PluginManager& pluginManager);

	/// Registers a pull proposed block handler in \a handlers constructing response from \a pFsmWeak.
	void RegisterPullProposedBlockHandler(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		ionet::ServerPacketHandlers& handlers);

	/// Registers a push confirmed block handler in \a handlers verifying data with \a pFsmWeak and \a pluginManager.
	void RegisterPushConfirmedBlockHandler(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		ionet::ServerPacketHandlers& handlers,
		const plugins::PluginManager& pluginManager);

	/// Registers a pull confirmed block handler in \a handlers constructing response from \a pFsmWeak.
	void RegisterPullConfirmedBlockHandler(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		ionet::ServerPacketHandlers& handlers);

	/// Registers a push prevote message handler in \a handlers constructing response from \a pFsmWeak.
	void RegisterPushPrevoteMessagesHandler(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		ionet::ServerPacketHandlers& handlers);

	/// Registers a push precommit message handler in \a handlers that adds the message to \a pFsmWeak.
	void RegisterPushPrecommitMessagesHandler(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		ionet::ServerPacketHandlers& handlers);

	/// Registers a pull prevote message handler in \a handlers constructing response from \a pFsmWeak.
	void RegisterPullPrevoteMessagesHandler(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		ionet::ServerPacketHandlers& handlers);

	/// Registers a pull precommit message handler in \a handlers constructing response from \a pFsmWeak.
	void RegisterPullPrecommitMessagesHandler(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		ionet::ServerPacketHandlers& handlers);

	/// Registers a pull remote node state handler in \a handlers constructing response from \a pFsmWeak
	/// using \a pConfigHolder, \a blockElementGetter and \a lastBlockElementSupplier.
	void RegisterPullRemoteNodeStateHandler(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		ionet::ServerPacketHandlers& handlers,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const std::function<std::shared_ptr<const model::BlockElement> (const Height&)>& blockElementGetter,
		const model::BlockElementSupplier& lastBlockElementSupplier);
}}