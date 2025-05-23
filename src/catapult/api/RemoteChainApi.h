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

#pragma once
#include "ChainApi.h"
#include "RemoteApi.h"
#include "catapult/model/CacheEntryInfo.h"

namespace catapult {
	namespace ionet { class PacketIo; }
	namespace model { class TransactionRegistry; }
}

namespace catapult { namespace api {

	/// Options for a blocks-from request.
	struct BlocksFromOptions {
		/// Creates blocks-from options.
		BlocksFromOptions() : BlocksFromOptions(0, 0)
		{}

		/// Creates blocks-from options from a number of requested blocks (\a numBlocks)
		/// and a number of requested bytes (\a numBytes).
		BlocksFromOptions(uint32_t numBlocks, uint32_t numBytes)
				: NumBlocks(numBlocks)
				, NumBytes(numBytes)
		{}

		/// Requested number of blocks.
		uint32_t NumBlocks;

		/// Requested number of bytes.
		uint32_t NumBytes;
	};

	/// An api for retrieving chain information from a remote node.
	class RemoteChainApi : public RemoteApi, public ChainApi {
	protected:
		/// Creates a remote api for the node with specified public key (\a remotePublicKey).
		explicit RemoteChainApi(const Key& remotePublicKey) : RemoteApi(remotePublicKey)
		{}

	public:
		/// Gets the last block.
		virtual thread::future<std::shared_ptr<const model::Block>> blockLast() const = 0;

		/// Gets the block at \a height.
		virtual thread::future<std::shared_ptr<const model::Block>> blockAt(Height height) const = 0;

		/// Gets the blocks starting at \a height with the specified \a options.
		/// \note An empty range will be returned if remote chain height is less than \a height.
		virtual thread::future<model::BlockRange> blocksFrom(Height height, const BlocksFromOptions& options) const = 0;

		/// Gets network config entries for all \a heights.
		virtual thread::future<model::EntityRange<model::CacheEntryInfo<Height>>> networkConfigs(
				model::EntityRange<Height>&& heights) const = 0;
	};

	/// Creates a chain api for interacting with a remote node with the specified \a io.
	std::unique_ptr<ChainApi> CreateRemoteChainApiWithoutRegistry(ionet::PacketIo& io);

	/// Creates a chain api for interacting with a remote node with the specified \a io with public key (\a remotePublicKey)
	/// and transaction \a registry composed of supported transactions.
	std::unique_ptr<RemoteChainApi> CreateRemoteChainApi(
			ionet::PacketIo& io,
			const Key& remotePublicKey,
			const model::TransactionRegistry& registry);
}}
