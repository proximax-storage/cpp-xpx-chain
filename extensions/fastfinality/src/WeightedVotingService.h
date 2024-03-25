/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "dbrb/DbrbConfiguration.h"
#include "catapult/extensions/ServiceRegistrar.h"
#include "catapult/harvesting_core/HarvestingConfiguration.h"

namespace catapult { namespace dbrb { class TransactionSender; }}

namespace catapult { namespace fastfinality {

	/// Creates a registrar for a weighted voting service.
	DECLARE_SERVICE_REGISTRAR(WeightedVoting)(
		const harvesting::HarvestingConfiguration& harvestingConfig,
		const dbrb::DbrbConfiguration& dbrbConfig,
		std::shared_ptr<dbrb::TransactionSender> pTransactionSender);

	/// Creates a registrar for a weighted voting shutdown service.
	DECLARE_SERVICE_REGISTRAR(WeightedVotingShutdown)(std::shared_ptr<dbrb::TransactionSender> pTransactionSender);
}}
