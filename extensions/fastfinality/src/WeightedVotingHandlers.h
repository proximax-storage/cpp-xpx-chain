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

	/// A retriever that returns the block hashes at given height from a number of peers and attaches packet io pair to each hash.
	using RemoteBlockHashesIoRetriever = std::function<thread::future<std::vector<std::pair<Hash256, std::shared_ptr<ionet::NodePacketIoPair>>>> (size_t, const Height&)>;

	/// A retriever that returns the network committee stages from a number of peers.
	using RemoteCommitteeStagesRetriever = std::function<thread::future<std::vector<CommitteeStage>> (size_t)>;

	/// A retriever that returns the proposed block from a number of peers.
	using RemoteProposedBlockRetriever = std::function<thread::future<std::vector<std::shared_ptr<model::Block>>> (size_t)>;

	/// Registers a committee stage pull handler in \a handlers constructing response from \a pFsm.
	void RegisterPullCommitteeStageHandler(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		ionet::ServerPacketHandlers& handlers);

	/// Registers a push proposed block handler in \a handlers verifying data with \a pFsm and \a pluginManager.
	void RegisterPushProposedBlockHandler(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		ionet::ServerPacketHandlers& handlers,
		const plugins::PluginManager& pluginManager,
		const extensions::PacketPayloadSink& packetPayloadSink);

	/// Registers a pull proposed block handler in \a handlers verifying data with \a pFsm and \a pluginManager.
	void RegisterPullProposedBlockHandler(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		ionet::ServerPacketHandlers& handlers);

	/// Registers a push prevote message handler in \a handlers constructing response from \a pFsm.
	void RegisterPushPrevoteMessageHandler(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		ionet::ServerPacketHandlers& handlers,
		const extensions::PacketPayloadSink& packetPayloadSink);

	/// Registers a push precommit message handler in \a handlers that adds the message to \a pFsm.
	void RegisterPushPrecommitMessageHandler(
		const std::shared_ptr<WeightedVotingFsm>& pFsm,
		ionet::ServerPacketHandlers& handlers,
		const extensions::PacketPayloadSink& packetPayloadSink);
}}