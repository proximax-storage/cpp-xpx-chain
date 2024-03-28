/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "StorageBlockchainBuilder.h"
#include "StorageBlockchain.h"

namespace catapult { namespace contract {

	StorageBlockchainBuilder::StorageBlockchainBuilder(const state::ContractState& contractState)
		: m_contractState(contractState) {}

	std::shared_ptr<Blockchain> StorageBlockchainBuilder::build(GlobalEnvironment& environment) {
		return std::make_shared<StorageBlockchain>(m_contractState);
	}

}} // namespace catapult::contract