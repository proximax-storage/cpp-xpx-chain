/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include <common/ServiceBuilder.h>
#include <blockchain/Blockchain.h>
#include <catapult/state/ContractState.h>

using namespace sirius::contract;
using namespace sirius::contract::blockchain;

namespace catapult { namespace contract {

	class StorageBlockchainBuilder : public ServiceBuilder<Blockchain> {

	public:

		StorageBlockchainBuilder(const state::ContractState& contractState);

		std::shared_ptr<Blockchain> build(GlobalEnvironment& environment) override;

	private:

		const state::ContractState& m_contractState;

	};

}}