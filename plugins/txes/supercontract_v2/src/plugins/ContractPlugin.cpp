/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ContractPlugin.h"
#include "src/cache/SuperContractCacheStorage.h"
#include "src/cache/DriveContractCacheStorage.h"
#include "src/plugins/DeployContractTransactionPlugin.h"
#include "src/plugins/AutomaticExecutionsPaymentTransactionPlugin.h"
#include "src/plugins/ManualCallTransactionPlugin.h"
#include "src/plugins/SuccessfulEndBatchExecutionTransactionPlugin.h"
#include "src/plugins/UnsuccessfulEndBatchExecutionTransactionPlugin.h"
#include "src/observers/SuperContractStorageUpdatesListener.h"
#include "src/plugins/EndBatchExecutionSingleTransactionPlugin.h"
#include "src/plugins/SynchronizationSingleTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "src/observers/Observers.h"
#include "catapult/plugins/CacheHandlers.h"
#include <catapult/model/SupercontractNotifications.h>
#include <src/state/ContractStateImpl.h>
#include "src/model/EndBatchExecutionSingleTransaction.h"
#include "src/model/SuccessfulEndBatchExecutionTransaction.h"
#include "src/model/UnsuccessfulEndBatchExecutionTransaction.h"
#include "src/model/SynchronizationSingleTransaction.h"


namespace catapult { namespace plugins {

	namespace {
		template<typename TUnresolvedData>
		const TUnresolvedData* castToUnresolvedData(const UnresolvedAmountData* pData) {
			if (!pData)
				CATAPULT_THROW_RUNTIME_ERROR("unresolved amount data pointer is null")

			auto pCast = dynamic_cast<const TUnresolvedData*>(pData);
			if (!pCast)
				CATAPULT_THROW_RUNTIME_ERROR("unresolved amount data pointer is of unexpected type")

			return pCast;
		}

		template<class TTransactionBody>
		void setUnlimitedTransactionFee(model::TransactionFeeCalculator& feeCalculator) {
			for (VersionType i = 1; i <= TTransactionBody::Current_Version; i++) {
				feeCalculator.addUnlimitedFeeTransaction(TTransactionBody::Entity_Type, i);
			}
		}
	}

	void RegisterContractSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::SuperContractConfiguration>();
		});

		const auto& pConfigHolder = manager.configHolder();
		const auto& immutableConfig = manager.immutableConfig();
		manager.addTransactionSupport(CreateDeployContractTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateAutomaticExecutionsPaymentTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateManualCallTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateSuccessfulEndBatchExecutionTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateUnsuccessfulEndBatchExecutionTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateEndBatchExecutionSingleTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateSynchronizationSingleTransactionPlugin(immutableConfig));

		auto& transactionFeeCalculator = *manager.transactionFeeCalculator();
		setUnlimitedTransactionFee<model::EndBatchExecutionSingleTransaction>(transactionFeeCalculator);
		setUnlimitedTransactionFee<model::SuccessfulEndBatchExecutionTransaction>(transactionFeeCalculator);
		setUnlimitedTransactionFee<model::UnsuccessfulEndBatchExecutionTransaction>(transactionFeeCalculator);
		setUnlimitedTransactionFee<model::SynchronizationSingleTransaction>(transactionFeeCalculator);

		auto& pBrowser = manager.driveStateBrowser();

		manager.addAmountResolver([&](const auto& cache, const auto& unresolved, auto& resolved) {
			switch (unresolved.Type) {
			case UnresolvedAmountType::ExecutorWork: {
				const auto* pWork = castToUnresolvedData<model::ExecutorWork>(unresolved.DataPtr);

				const auto& contractCache = cache.template sub<cache::SuperContractCache>();
				auto contractIter = contractCache.find(pWork->ContractKey);
				const state::SuperContractEntry& contractEntry = contractIter.get();

				auto executorsNumber = pBrowser->getOrderedReplicatorsCount(cache, contractEntry.driveKey());

				resolved = Amount(pWork->AmountPerExecutor.unwrap() * executorsNumber);
				return true;
			}
			case UnresolvedAmountType::AutomaticExecutionWork: {
				const auto* pWork = castToUnresolvedData<model::AutomaticExecutorWork>(unresolved.DataPtr);

				const auto& contractCache = cache.template sub<cache::SuperContractCache>();
				auto contractIter = contractCache.find(pWork->ContractKey);
				const state::SuperContractEntry& contractEntry = contractIter.get();

				auto executorsNumber = pBrowser->getOrderedReplicatorsCount(cache, contractEntry.driveKey());

				resolved =
						Amount(pWork->NumberOfExecutions *
							   contractEntry.automaticExecutionsInfo().AutomaticExecutionCallPayment.unwrap() *
							   executorsNumber);
				return true;
			}
			case UnresolvedAmountType::AutomaticDownloadWork: {
				const auto* pWork = castToUnresolvedData<model::AutomaticExecutorWork>(unresolved.DataPtr);

				const auto& contractCache = cache.template sub<cache::SuperContractCache>();
				auto contractIter = contractCache.find(pWork->ContractKey);
				const state::SuperContractEntry& contractEntry = contractIter.get();

				auto executorsNumber = pBrowser->getOrderedReplicatorsCount(cache, contractEntry.driveKey());

				resolved =
						Amount(pWork->NumberOfExecutions *
						contractEntry.automaticExecutionsInfo().AutomaticDownloadCallPayment.unwrap() *
						executorsNumber);
				return true;
			}
		  	default:
			  	break;
			}

			return false;
		});

		manager.addCacheSupport<cache::SuperContractCacheStorage>(
				std::make_unique<cache::SuperContractCache>(manager.cacheConfig(cache::SuperContractCache::Name), pConfigHolder));

		using SuperContractCacheHandlersService = CacheHandlers<cache::SuperContractCacheDescriptor>;
		SuperContractCacheHandlersService::Register<model::FacilityCode::SuperContractCache>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("SUPER CONTRACT C"), [&cache]() {
				return cache.sub<cache::SuperContractCache>().createView(cache.height())->size();
			});
		});

		using DriveContractCacheHandlersService = CacheHandlers<cache::DriveContractCacheDescriptor>;
		DriveContractCacheHandlersService::Register<model::FacilityCode::DriveContractCache>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("DRIVE CONTRACT C"), [&cache]() {
				return cache.sub<cache::DriveContractCache>().createView(cache.height())->size();
			});
		});

		const auto& liquidityProviderValidator = manager.liquidityProviderExchangeValidator();
		const auto& liquidityProviderObserver = manager.liquidityProviderExchangeObserver();

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateSuperContractPluginConfigValidator());
		});

		manager.addStatefulValidatorHook([pConfigHolder, &immutableConfig](auto& builder) {
		  	builder
		  		.add(validators::CreateManualCallValidator())
		  		.add(validators::CreateAutomaticExecutionsReplenishmentValidator())
				.add(validators::CreateDeployContractValidator())
				.add(validators::CreateEndBatchExecutionValidator())
				.add(validators::CreateBatchCallsValidator())
				.add(validators::CreateContractStateUpdateValidator())
				.add(validators::CreateOpinionSignatureValidator())
				.add(validators::CreateProofOfExecutionValidator())
				.add(validators::CreateEndBatchExecutionSingleValidator())
				.add(validators::CreateSynchronizationSingleValidator())
				.add(validators::CreateReleasedTransactionsValidator());
		});

		const auto& storageExternalManagement = manager.storageExternalManagement();
		const auto& driveStateBrowser = manager.driveStateBrowser();

		auto pStorageUpdatesListener = std::make_unique<observers::SuperContractStorageUpdatesListener>(
				driveStateBrowser, liquidityProviderObserver);
		manager.addStorageUpdateListener(std::move(pStorageUpdatesListener));

		auto pContractState = std::make_shared<state::ContractStateImpl>(driveStateBrowser);
		manager.setContractState(pContractState);

		manager.addObserverHook([&](auto& builder) {
			builder
				.add(observers::CreateDeployContractObserver(driveStateBrowser))
				.add(observers::CreateAutomaticExecutionsReplenishmentObserver())
				.add(observers::CreateManualCallObserver())
				.add(observers::CreateProofOfExecutionObserver(
							liquidityProviderObserver, storageExternalManagement))
				.add(observers::CreateBatchCallsObserver(liquidityProviderObserver, driveStateBrowser))
				.add(observers::CreateContractStateUpdateObserver(driveStateBrowser))
				.add(observers::CreateContractDestroyObserver(storageExternalManagement))
				.add(observers::CreateEndBatchExecutionObserver(driveStateBrowser))
				.add(observers::CreateSuccessfulEndBatchExecutionObserver(storageExternalManagement))
				.add(observers::CreateUnsuccessfulEndBatchExecutionObserver())
				.add(observers::CreateSynchronizationSingleObserver(storageExternalManagement))
				.add(observers::CreateReleasedTransactionsObserver());
		});
	}
}}


extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterContractSubsystem(manager);
}
