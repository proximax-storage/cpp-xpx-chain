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

#include "src/AddressExtractionBlockChangeSubscriber.h"
#include "src/AddressExtractionPtChangeSubscriber.h"
#include "src/AddressExtractionUtChangeSubscriber.h"
#include "src/AddressExtractor.h"
#include "catapult/extensions/ProcessBootstrapper.h"
#include "catapult/extensions/RootedService.h"

namespace catapult { namespace addressextraction {

	namespace {
		void RegisterExtension(extensions::ProcessBootstrapper& bootstrapper) {
			
			auto pAddressExtractor = std::make_shared<AddressExtractor>(
				bootstrapper.pluginManager().createNotificationPublisher(),
				[&bootstrapper]() {
					return bootstrapper.pluginManager().createExtractorContext(bootstrapper.cacheHolder().cache());
				},
				[&bootstrapper]() {
					return util::ResolverContextHandle(
						[&bootstrapper](){
							return bootstrapper.cacheHolder().cache().createView().toReadOnly();
						},
						[&bootstrapper](auto& cache){
							return bootstrapper.pluginManager().createResolverContext(cache);
						});
				});

			// add a dummy service for extending service lifetimes
			bootstrapper.extensionManager().addServiceRegistrar(extensions::CreateRootedServiceRegistrar(
					pAddressExtractor,
					"addressextraction.extractor",
					extensions::ServiceRegistrarPhase::Initial));

			// register subscriber
			auto& subscriptionManager = bootstrapper.subscriptionManager();
			subscriptionManager.addBlockChangeSubscriber(CreateAddressExtractionBlockChangeSubscriber(*pAddressExtractor));
			subscriptionManager.addUtChangeSubscriber(CreateAddressExtractionUtChangeSubscriber(*pAddressExtractor));
			subscriptionManager.addPtChangeSubscriber(CreateAddressExtractionPtChangeSubscriber(*pAddressExtractor));
		}
	}
}}

extern "C" PLUGIN_API
void RegisterExtension(catapult::extensions::ProcessBootstrapper& bootstrapper) {
	catapult::addressextraction::RegisterExtension(bootstrapper);
}
