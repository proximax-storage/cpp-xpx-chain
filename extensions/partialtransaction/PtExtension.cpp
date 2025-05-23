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

#include "src/PtBootstrapperService.h"
#include "src/PtConfiguration.h"
#include "src/PtDispatcherService.h"
#include "src/PtService.h"
#include "src/PtSyncSourceService.h"
#include "catapult/extensions/ProcessBootstrapper.h"

namespace catapult { namespace partialtransaction {

	namespace {
		void RegisterExtension(extensions::ProcessBootstrapper& bootstrapper) {
			const auto& resourcesPath = bootstrapper.resourcesPath();
			auto config = PtConfiguration::LoadFromPath(resourcesPath);
			auto ptCacheOptions = cache::MemoryCacheOptions(config.CacheMaxResponseSize.bytes(), config.CacheMaxSize);

			// create and register the pt cache (it is optional, so not in server state)
			auto& extensionManager = bootstrapper.extensionManager();
			auto& subscriptionManager = bootstrapper.subscriptionManager();
			extensionManager.addServiceRegistrar(CreatePtBootstrapperServiceRegistrar([&subscriptionManager,
																					   ptCacheOptions,
																					   feeCalculator=bootstrapper.pluginManager().transactionFeeCalculator()]() {
				return subscriptionManager.createPtCache(ptCacheOptions, feeCalculator);
			}));

			// register other services
			extensionManager.addServiceRegistrar(CreatePtDispatcherServiceRegistrar());
			extensionManager.addServiceRegistrar(CreatePtSyncSourceServiceRegistrar());
			extensionManager.addServiceRegistrar(CreatePtServiceRegistrar());
		}
	}
}}

extern "C" PLUGIN_API
void RegisterExtension(catapult::extensions::ProcessBootstrapper& bootstrapper) {
	catapult::partialtransaction::RegisterExtension(bootstrapper);
}
