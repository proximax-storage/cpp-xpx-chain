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

#include "AggregatePlugin.h"
#include "AggregateTransactionPlugin.h"
#include "src/model/AggregateEntityType.h"
#include "src/model/AggregateTransaction.h"
#include "src/validators/Validators.h"
#include "src/observers/Observers.h"
#include "catapult/plugins/PluginManager.h"

namespace catapult { namespace plugins {

	namespace {
		constexpr auto FeeAggregateBondedFixVersion = catapult::BlockchainVersion{
			uint64_t{1}	<< 48 |
			uint64_t{5}	<< 32 |
			uint64_t{1}	<< 16 |
			uint64_t{0}
		};
	}

	void RegisterAggregateSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::AggregateConfiguration>();
		});

		manager.transactionFeeCalculator()->addTransactionSizeSupplier(model::Entity_Type_Aggregate_Bonded, [pConfigHolder = manager.configHolder()](const model::Transaction& transaction, const Height& height) {
			auto size = transaction.Size;
			if (pConfigHolder->Version(height) >= FeeAggregateBondedFixVersion)
				size -= reinterpret_cast<const model::AggregateTransaction&>(transaction).CosignaturesCount() * sizeof(model::Cosignature);

			return size;
		});

		// configure the aggregate to allow all registered transactions that support embedding
		// (this works because the transaction registry is held by reference)
		const auto& transactionRegistry = manager.transactionRegistry();
		const auto& pConfigHolder = manager.configHolder();
		manager.addTransactionSupport(CreateAggregateTransactionPlugin(transactionRegistry, model::Entity_Type_Aggregate_Complete, pConfigHolder));
		manager.addTransactionSupport(CreateAggregateTransactionPlugin(transactionRegistry, model::Entity_Type_Aggregate_Bonded, pConfigHolder));

		manager.addStatelessValidatorHook([](auto& builder) {
			builder.add(validators::CreateAggregatePluginConfigValidator());
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateBasicAggregateCosignaturesValidator())
				.add(validators::CreateStrictAggregateCosignaturesValidator())
				.add(validators::CreateAggregateTransactionTypeValidator())
				.add(validators::CreateStrictAggregateTransactionTypeValidator())
				.add(validators::CreateTransactionFeeCompensationValidator());
		});

		manager.addObserverHook([] (auto& builder) {
			builder.add(observers::CreateTransactionFeeCompensationObserver());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterAggregateSubsystem(manager);
}
