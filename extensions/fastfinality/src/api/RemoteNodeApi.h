/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "fastfinality/src/WeightedVotingChainPackets.h"
#include "catapult/types.h"
#include "catapult/thread/Future.h"

namespace catapult {
	namespace ionet { class PacketIo; }
	namespace model {
		class Block;
		class TransactionRegistry;
	}
}

namespace catapult { namespace fastfinality {

	/// An api for retrieving committee information from a remote node.
	class RemoteNodeApi {
	public:
		virtual ~RemoteNodeApi() = default;

	public:
		/// Gets a block proposed by current block producer.
		virtual thread::future<std::shared_ptr<model::Block>> proposedBlock() const = 0;

		/// Gets a block confirmed by current committee.
		virtual thread::future<std::shared_ptr<model::Block>> confirmedBlock() const = 0;

		/// Gets prevote messages.
		virtual thread::future<std::vector<CommitteeMessage>> prevotes() const = 0;

		/// Gets precommit messages.
		virtual thread::future<std::vector<CommitteeMessage>> precommits() const = 0;
	};

	/// Creates a node api for interacting with a remote node with the specified \a io
	/// and transaction \a registry composed of supported transactions.
	std::unique_ptr<RemoteNodeApi> CreateRemoteNodeApi(ionet::PacketIo& io, const model::TransactionRegistry& registry);

	/// Creates a node api for interacting with a remote node with the specified \a io.
	std::unique_ptr<RemoteNodeApi> CreateRemoteNodeApiWithoutRegistry(ionet::PacketIo& io);
}}
