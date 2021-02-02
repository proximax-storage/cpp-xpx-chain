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

	/// A retriever that returns the network chain heights for all available peers.
	using RemoteChainHeightsRetriever = std::function<thread::future<std::vector<Height>> ()>;

	/// A retriever that returns the block hashes at given height from all available peers and attaches node info to each hash.
	using RemoteBlockHashesRetriever = std::function<thread::future<std::vector<std::pair<Hash256, Key>>> (const Height&)>;

	/// A retriever that returns the network committee stages from all available peers.
	using RemoteCommitteeStagesRetriever = std::function<thread::future<std::vector<CommitteeStage>> ()>;

	/// Registers a committee stage pull handler in \a handlers constructing response from \a pFsmWeak.
	void RegisterPullCommitteeStageHandler(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		ionet::ServerPacketHandlers& handlers);

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
}}