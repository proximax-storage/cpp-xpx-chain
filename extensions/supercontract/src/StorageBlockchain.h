/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include <blockchain/Blockchain.h>
#include <catapult/thread/IoThreadPool.h>
#include <catapult/state/ContractState.h>

using namespace sirius::contract;
using namespace sirius::contract::blockchain;

namespace catapult { namespace contract {

	class StorageBlockchain : public Blockchain,
							  public std::enable_shared_from_this<StorageBlockchain> {

	public:

		StorageBlockchain(const state::ContractState& contractState);

		void block(uint64_t height, std::shared_ptr<AsyncQueryCallback<Block>> callback) override;

	private:

		const state::ContractState& m_contractState;
		sirius::contract::ThreadManager m_threadManager;

	};

}}