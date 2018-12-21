/**
*** Copyright (c) 2018-present,
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

#include "catapult/handlers/CacheEntryInfosProducerFactory.h"
#include "catapult/handlers/StatePathHandlerFactory.h"
#include "catapult/plugins/PluginManager.h"
#include "ContractPlugin.h"
#include "src/cache/ContractCache.h"
#include "src/cache/ContractCacheStorage.h"
#include "src/cache/ReputationCache.h"
#include "src/cache/ReputationCacheStorage.h"
#include "src/handlers/ContractDiagnosticHandlers.h"
#include "src/handlers/ReputationDiagnosticHandlers.h"
#include "src/observers/Observers.h"
#include "src/plugins/ModifyContractTransactionPlugin.h"
#include "src/validators/Validators.h"

namespace catapult { namespace plugins {

	void RegisterContractSubsystem(PluginManager& manager) {
		manager.addTransactionSupport(CreateModifyContractTransactionPlugin());

		manager.addCacheSupport<cache::ContractCacheStorage>(
			std::make_unique<cache::ContractCache>(manager.cacheConfig(cache::ContractCache::Name)));

		manager.addCacheSupport<cache::ReputationCacheStorage>(
			std::make_unique<cache::ReputationCache>(manager.cacheConfig(cache::ReputationCache::Name)));

		manager.addDiagnosticHandlerHook([](auto& handlers, const cache::CatapultCache& cache) {
			using ContractInfosProducerFactory = handlers::CacheEntryInfosProducerFactory<cache::ContractCacheDescriptor>;
			handlers::RegisterContractInfosHandler(handlers, ContractInfosProducerFactory::Create(cache.sub<cache::ContractCache>()));

			using ContractPacketType = handlers::StatePathRequestPacket<ionet::PacketType::Contract_State_Path, Key>;
			handlers::RegisterStatePathHandler<ContractPacketType>(handlers, cache.sub<cache::ContractCache>());

			using ReputationInfosProducerFactory = handlers::CacheEntryInfosProducerFactory<cache::ReputationCacheDescriptor>;
			handlers::RegisterReputationInfosHandler(handlers, ReputationInfosProducerFactory::Create(cache.sub<cache::ReputationCache>()));

			using ReputationPacketType = handlers::StatePathRequestPacket<ionet::PacketType::Reputation_State_Path, Key>;
			handlers::RegisterStatePathHandler<ReputationPacketType>(handlers, cache.sub<cache::ReputationCache>());
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder.add(validators::CreateModifyContractCustomersValidator());
			builder.add(validators::CreateModifyContractExecutorsValidator());
			builder.add(validators::CreateModifyContractVerifiersValidator());
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
					.add(validators::CreateModifyContractInvalidCustomersValidator())
					.add(validators::CreateModifyContractInvalidExecutorsValidator())
					.add(validators::CreateModifyContractInvalidVerifiersValidator())
					.add(validators::CreateModifyContractDurationValidator());
		});

		manager.addObserverHook([](auto& builder) {
			builder.add(observers::CreateModifyContractObserver());
			builder.add(observers::CreateReputationUpdateObserver());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterContractSubsystem(manager);
}
