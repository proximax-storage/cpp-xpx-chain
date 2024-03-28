/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StorageBlockchain.h"
#include <boost/asio.hpp>
#include <catapult/utils/NetworkTime.h>

namespace catapult { namespace contract {

	StorageBlockchain::StorageBlockchain(const state::ContractState& contractState)
		: m_contractState(contractState) {}

	void StorageBlockchain::block(uint64_t height, std::shared_ptr<AsyncQueryCallback<Block>> callback) {
		m_threadManager.execute([this, height, callback=std::move(callback)] {
			auto storageBlock = m_contractState.getBlock(Height(height));
			Block block;
			block.m_blockHash = storageBlock->EntityHash.array();
			block.m_blockTime = utils::ToUnixTime(storageBlock->Block.Timestamp).unwrap();
			callback->postReply(std::move(block));
		});
	}
}}