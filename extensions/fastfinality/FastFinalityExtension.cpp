/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/WeightedVotingService.h"
#include "catapult/extensions/ProcessBootstrapper.h"
#include "catapult/harvesting_core/ValidateHarvestingConfiguration.h"

namespace catapult { namespace fastfinality {

	namespace {
		void RegisterExtension(extensions::ProcessBootstrapper& bootstrapper) {
			auto config = harvesting::HarvestingConfiguration::LoadFromPath(bootstrapper.resourcesPath());
			ValidateHarvestingConfiguration(config);

			bootstrapper.extensionManager().addServiceRegistrar(CreateWeightedVotingServiceRegistrar(config));
			bootstrapper.extensionManager().addServiceRegistrar(CreateWeightedVotingStartServiceRegistrar());
		}
	}
}}

extern "C" PLUGIN_API
void RegisterExtension(catapult::extensions::ProcessBootstrapper& bootstrapper) {
	catapult::fastfinality::RegisterExtension(bootstrapper);
}
