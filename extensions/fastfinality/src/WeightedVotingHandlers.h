/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/extensions/ServerHooks.h"
#include "catapult/plugins/PluginManager.h"
#include "catapult/thread/Future.h"
#include "catapult/types.h"
#include "WeightedVotingChainPackets.h"
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

	// TODO: Add proper description for RemoteNodeStateRetriever, double-check import
	///
	using RemoteNodeStateRetriever = std::function<thread::future<std::vector<RemoteNodeState>> ()>;

	/// Registers a push proposed block handler in \a handlers verifying data with \a pFsmWeak and \a pluginManager.
	void RegisterPushProposedBlockHandler(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		ionet::ServerPacketHandlers& handlers,
		const plugins::PluginManager& pluginManager);

	/// Registers a push confirmed block handler in \a handlers verifying data with \a pFsmWeak and \a pluginManager.
	void RegisterPushConfirmedBlockHandler(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		ionet::ServerPacketHandlers& handlers,
		const plugins::PluginManager& pluginManager);

	/// Registers a push prevote message handler in \a handlers constructing response from \a pFsmWeak.
	void RegisterPushPrevoteMessageHandler(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		ionet::ServerPacketHandlers& handlers);

	/// Registers a push precommit message handler in \a handlers that adds the message to \a pFsmWeak.
	void RegisterPushPrecommitMessageHandler(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		ionet::ServerPacketHandlers& handlers);

	// TODO: Add proper description for RegisterPullRemoteNodeStateHandler
	///
	void RegisterPullRemoteNodeStateHandler(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		ionet::ServerPacketHandlers& handlers,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		const std::function<std::shared_ptr<const model::BlockElement> (const Height&)>& blockElementGetter,
		const model::BlockElementSupplier& lastBlockElementSupplier);
}}