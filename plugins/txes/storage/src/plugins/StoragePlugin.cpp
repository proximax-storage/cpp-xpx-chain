/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StoragePlugin.h"
#include "src/cache/QueueCacheStorage.h"
#include "src/cache/PriorityQueueCache.h"
#include "src/cache/PriorityQueueCacheStorage.h"
#include "src/cache/BcDriveCacheStorage.h"
#include "src/cache/DownloadChannelCacheStorage.h"
#include "src/cache/ReplicatorCacheStorage.h"
#include "src/cache/BootKeyReplicatorCache.h"
#include "src/cache/BootKeyReplicatorCacheStorage.h"
#include "src/plugins/PrepareBcDriveTransactionPlugin.h"
#include "src/plugins/DataModificationTransactionPlugin.h"
#include "src/plugins/DownloadTransactionPlugin.h"
#include "src/plugins/DataModificationApprovalTransactionPlugin.h"
#include "src/plugins/DataModificationCancelTransactionPlugin.h"
#include "src/plugins/ReplicatorOnboardingTransactionPlugin.h"
#include "src/plugins/DriveClosureTransactionPlugin.h"
#include "src/plugins/ReplicatorOffboardingTransactionPlugin.h"
#include "src/plugins/FinishDownloadTransactionPlugin.h"
#include "src/plugins/DownloadPaymentTransactionPlugin.h"
#include "src/plugins/StoragePaymentTransactionPlugin.h"
#include "src/plugins/DataModificationSingleApprovalTransactionPlugin.h"
#include "src/plugins/VerificationPaymentTransactionPlugin.h"
#include "src/plugins/DownloadApprovalTransactionPlugin.h"
#include "src/plugins/EndDriveVerificationTransactionPlugin.h"
#include "src/plugins/ReplicatorsCleanupTransactionPlugin.h"
#include "src/state/StorageStateImpl.h"
#include "src/validators/Validators.h"
#include "src/observers/Observers.h"
#include "catapult/plugins/CacheHandlers.h"
#include "src/model/DataModificationApprovalTransaction.h"
#include "src/model/DataModificationSingleApprovalTransaction.h"
#include "src/model/DownloadApprovalTransaction.h"
#include "src/model/EndDriveVerificationTransaction.h"

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
				feeCalculator.addLimitedFeeTransaction(TTransactionBody::Entity_Type, i);
			}
		}
	}

	void RegisterStorageSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::StorageConfiguration>();
		});

		const auto& pConfigHolder = manager.configHolder();
		const auto& immutableConfig = manager.immutableConfig();
		manager.addTransactionSupport(CreatePrepareBcDriveTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateDataModificationTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateDownloadTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateDataModificationApprovalTransactionPlugin());
		manager.addTransactionSupport(CreateDataModificationCancelTransactionPlugin());
		manager.addTransactionSupport(CreateReplicatorOnboardingTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateDriveClosureTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateReplicatorOffboardingTransactionPlugin());
		manager.addTransactionSupport(CreateFinishDownloadTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateDownloadPaymentTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateStoragePaymentTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateDataModificationSingleApprovalTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateVerificationPaymentTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateDownloadApprovalTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateEndDriveVerificationTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateReplicatorsCleanupTransactionPlugin());

		auto transactionFeeCalculator = manager.transactionFeeCalculator();
		setUnlimitedTransactionFee<model::DataModificationApprovalTransaction>(*transactionFeeCalculator);
		setUnlimitedTransactionFee<model::DataModificationSingleApprovalTransaction>(*transactionFeeCalculator);
		setUnlimitedTransactionFee<model::DownloadApprovalTransaction>(*transactionFeeCalculator);
		setUnlimitedTransactionFee<model::EndDriveVerificationTransaction>(*transactionFeeCalculator);

		manager.addAmountResolver([](const auto& cache, const auto& unresolved, auto& resolved) {
			switch (unresolved.Type) {
			case UnresolvedAmountType::StreamingWork: {
				const auto& pStreamingWork = castToUnresolvedData<model::StreamingWork>(unresolved.DataPtr);

				const auto& driveCache = cache.template sub<cache::BcDriveCache>();
				const auto driveIter = driveCache.find(pStreamingWork->DriveKey);
				const auto& pDriveEntry = driveIter.tryGet();

				if (!pDriveEntry)
					break;

				resolved = Amount(2 * pStreamingWork->UploadSize * pDriveEntry->replicatorCount());
				return true;
			}
		  	default:
			  	break;
			}

			return false;
		});

		manager.addCacheSupport<cache::BcDriveCacheStorage>(
			std::make_unique<cache::BcDriveCache>(manager.cacheConfig(cache::BcDriveCache::Name), pConfigHolder));

		using BcDriveCacheHandlersService = CacheHandlers<cache::BcDriveCacheDescriptor>;
		BcDriveCacheHandlersService::Register<model::FacilityCode::BcDrive>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("BC DRIVE C"), [&cache]() {
				return cache.sub<cache::BcDriveCache>().createView(cache.height())->size();
			});
		});

		manager.addCacheSupport<cache::BootKeyReplicatorCacheStorage>(
			std::make_unique<cache::BootKeyReplicatorCache>(manager.cacheConfig(cache::BootKeyReplicatorCache::Name), pConfigHolder));

		using BootKeyReplicatorCacheHandlersService = CacheHandlers<cache::BootKeyReplicatorCacheDescriptor>;
		BootKeyReplicatorCacheHandlersService::Register<model::FacilityCode::BootKeyReplicator>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("BOOTKEYREP C"), [&cache]() {
				return cache.sub<cache::BootKeyReplicatorCache>().createView(cache.height())->size();
			});
		});

		manager.addCacheSupport<cache::DownloadChannelCacheStorage>(
			std::make_unique<cache::DownloadChannelCache>(manager.cacheConfig(cache::DownloadChannelCache::Name), pConfigHolder));

		using DownloadChannelCacheHandlersService = CacheHandlers<cache::DownloadChannelCacheDescriptor>;
		DownloadChannelCacheHandlersService::Register<model::FacilityCode::DownloadChannel>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("DOWNLOAD CH C"), [&cache]() {
				return cache.sub<cache::DownloadChannelCache>().createView(cache.height())->size();
			});
		});

		manager.addCacheSupport<cache::QueueCacheStorage>(
				std::make_unique<cache::QueueCache>(manager.cacheConfig(cache::QueueCache::Name), pConfigHolder));

		using QueueCacheHandlersService = CacheHandlers<cache::QueueCacheDescriptor>;
		QueueCacheHandlersService::Register<model::FacilityCode::Queue>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("QUEUE C"), [&cache]() {
				return cache.sub<cache::QueueCache>().createView(cache.height())->size();
			});
		});

		manager.addCacheSupport<cache::PriorityQueueCacheStorage>(
				std::make_unique<cache::PriorityQueueCache>(manager.cacheConfig(cache::PriorityQueueCache::Name), pConfigHolder));

		using PriorityQueueCacheHandlersService = CacheHandlers<cache::PriorityQueueCacheDescriptor>;
		PriorityQueueCacheHandlersService::Register<model::FacilityCode::PriorityQueue>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
		 	counters.emplace_back(utils::DiagnosticCounterId("PR QUEUE C"), [&cache]() {
				return cache.sub<cache::PriorityQueueCache>().createView(cache.height())->size();
		  	});
		});

		manager.addCacheSupport<cache::ReplicatorCacheStorage>(
				std::make_unique<cache::ReplicatorCache>(manager.cacheConfig(cache::ReplicatorCache::Name), pConfigHolder));

		using ReplicatorCacheHandlersService = CacheHandlers<cache::ReplicatorCacheDescriptor>;
		ReplicatorCacheHandlersService::Register<model::FacilityCode::Replicator>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("REPLICATOR C"), [&cache]() {
				return cache.sub<cache::ReplicatorCache>().createView(cache.height())->size();
			});
		});

		auto pStorageState = std::make_shared<state::StorageStateImpl>();
		manager.setStorageState(pStorageState);

		const auto& liquidityProviderValidator = manager.liquidityProviderExchangeValidator();
		const auto& liquidityProviderObserver = manager.liquidityProviderExchangeObserver();

		manager.addDbrbProcessUpdateListener(std::make_unique<observers::StorageDbrbProcessUpdateListener>(liquidityProviderObserver, pStorageState));

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateStoragePluginConfigValidator());
		});

		manager.addStatefulValidatorHook([pConfigHolder, &immutableConfig](auto& builder) {
		  	builder
				.add(validators::CreatePrepareDriveValidator())
				.add(validators::CreateDataModificationValidator())
				.add(validators::CreateDataModificationApprovalValidator())
				.add(validators::CreateDataModificationApprovalDownloadWorkValidator())
				.add(validators::CreateDataModificationApprovalUploadWorkValidator())
				.add(validators::CreateDataModificationApprovalRefundValidator())
				.add(validators::CreateDataModificationCancelValidator())
				.add(validators::CreateDriveClosureValidator())
				.add(validators::CreateReplicatorOnboardingV1Validator())
				.add(validators::CreateReplicatorOnboardingV2Validator())
				.add(validators::CreateReplicatorOffboardingValidator())
				.add(validators::CreateFinishDownloadValidator())
				.add(validators::CreateDownloadPaymentValidator())
				.add(validators::CreateStoragePaymentValidator())
				.add(validators::CreateDataModificationSingleApprovalValidator())
		  		.add(validators::CreateVerificationPaymentValidator())
				.add(validators::CreateOpinionValidator())
				.add(validators::CreateDownloadChannelValidator())
				.add(validators::CreateDownloadApprovalValidator())
				.add(validators::CreateDownloadChannelRefundValidator())
				.add(validators::CreateStreamStartValidator())
				.add(validators::CreateStreamFinishValidator())
				.add(validators::CreateStreamPaymentValidator())
				.add(validators::CreateEndDriveVerificationValidator())
				.add(validators::CreateServiceUnitTransferValidator())
				.add(validators::CreateOwnerManagementProhibitionValidator())
				.add(validators::CreateReplicatorNodeBootKeyValidator())
				.add(validators::CreateReplicatorsCleanupV1Validator())
				.add(validators::CreateReplicatorsCleanupV2Validator());
		});

		const auto& storageUpdatesListeners = manager.storageUpdatesListeners();

		manager.addObserverHook([pStorageState, &liquidityProviderObserver, &storageUpdatesListeners](auto& builder) {
			builder
				.add(observers::CreatePrepareDriveObserver(pStorageState))
				.add(observers::CreateDownloadChannelObserver(pStorageState))
				.add(observers::CreateDataModificationObserver(liquidityProviderObserver, pStorageState))
				.add(observers::CreateDataModificationApprovalObserver(pStorageState))
				.add(observers::CreateDataModificationApprovalDownloadWorkObserver(liquidityProviderObserver, pStorageState))
				.add(observers::CreateDataModificationApprovalUploadWorkObserver(liquidityProviderObserver, pStorageState))
				.add(observers::CreateDataModificationApprovalRefundObserver(liquidityProviderObserver, pStorageState))
				.add(observers::CreateDataModificationCancelObserver(liquidityProviderObserver, pStorageState))
				.add(observers::CreateDriveClosureObserver(liquidityProviderObserver, storageUpdatesListeners, pStorageState))
				.add(observers::CreateReplicatorOnboardingV1Observer(pStorageState))
				.add(observers::CreateReplicatorOnboardingV2Observer(pStorageState))
				.add(observers::CreateReplicatorOffboardingObserver())
				.add(observers::CreateDownloadPaymentObserver(pStorageState))
				.add(observers::CreateDataModificationSingleApprovalObserver(pStorageState))
				.add(observers::CreateDownloadApprovalObserver(pStorageState))
				.add(observers::CreateFinishDownloadObserver(pStorageState))
				.add(observers::CreateDownloadApprovalPaymentObserver(liquidityProviderObserver, pStorageState))
				.add(observers::CreateDownloadChannelRefundObserver(liquidityProviderObserver, pStorageState))
				.add(observers::CreateStreamStartObserver(liquidityProviderObserver, pStorageState))
				.add(observers::CreateStreamFinishObserver(pStorageState))
				.add(observers::CreateStreamPaymentObserver(pStorageState))
				.add(observers::CreateStartDriveVerificationObserver(pStorageState))
				.add(observers::CreateEndDriveVerificationObserver(liquidityProviderObserver, pStorageState))
				.add(observers::CreatePeriodicStoragePaymentObserver(liquidityProviderObserver, storageUpdatesListeners, pStorageState))
				.add(observers::CreatePeriodicDownloadChannelPaymentObserver(pStorageState))
				.add(observers::CreateOwnerManagementProhibitionObserver())
				.add(observers::CreateReplicatorNodeBootKeyObserver())
				.add(observers::CreateReplicatorsCleanupV1Observer(liquidityProviderObserver))
				.add(observers::CreateReplicatorsCleanupV2Observer());
		});
	}
}}


extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterStorageSubsystem(manager);
}
