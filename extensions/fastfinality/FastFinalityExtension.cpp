/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/FastFinalityService.h"
#include "src/FastFinalityShutdownService.h"
#include "src/dbrb/TransactionSender.h"
#include "catapult/extensions/ProcessBootstrapper.h"
#include "catapult/harvesting_core/ValidateHarvestingConfiguration.h"

namespace catapult { namespace fastfinality {

	namespace {
		void RegisterExtension(extensions::ProcessBootstrapper& bootstrapper) {
			auto harvestingConfig = harvesting::HarvestingConfiguration::LoadFromPath(bootstrapper.resourcesPath());
			ValidateHarvestingConfiguration(harvestingConfig);
			auto dbrbConfig = dbrb::DbrbConfiguration::LoadFromPath(bootstrapper.resourcesPath());

			auto pTransactionSender = std::make_shared<dbrb::TransactionSender>();
			bootstrapper.subscriptionManager().addPostBlockCommitSubscriber(std::make_unique<dbrb::BlockSubscriber>(pTransactionSender));
			bootstrapper.extensionManager().addServiceRegistrar(CreateFastFinalityServiceRegistrar(harvestingConfig, dbrbConfig, pTransactionSender));
			bootstrapper.extensionManager().addServiceRegistrar(CreateFastFinalityShutdownServiceRegistrar(pTransactionSender));
		}
	}
}}

extern "C" PLUGIN_API
void RegisterExtension(catapult::extensions::ProcessBootstrapper& bootstrapper) {
	catapult::fastfinality::RegisterExtension(bootstrapper);
}
