/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StoragePlugin.h"
#include "src/cache/BcDriveCacheStorage.h"
#include "src/cache/DownloadChannelCacheStorage.h"
#include "src/cache/ReplicatorCacheSubCachePlugin.h"
#include "src/plugins/PrepareBcDriveTransactionPlugin.h"
#include "src/plugins/DataModificationTransactionPlugin.h"
#include "src/plugins/DownloadTransactionPlugin.h"
#include "src/plugins/DataModificationApprovalTransactionPlugin.h"
#include "src/plugins/DataModificationCancelTransactionPlugin.h"
#include "src/plugins/ReplicatorOnboardingTransactionPlugin.h"
#include "src/plugins/FinishDownloadTransactionPlugin.h"
#include "src/plugins/DownloadPaymentTransactionPlugin.h"
#include "src/plugins/StoragePaymentTransactionPlugin.h"
#include "src/plugins/DataModificationSingleApprovalTransactionPlugin.h"
#include "src/plugins/VerificationPaymentTransactionPlugin.h"
#include "src/plugins/FinishDriveVerificationTransactionPlugin.h"
#include "src/state/CachedStorageState.h"
#include "src/validators/Validators.h"
#include "src/observers/Observers.h"
#include "catapult/plugins/CacheHandlers.h"

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

		const auto calculateApprovableDownloadWork(const state::ReplicatorEntry* pReplicatorEntry, const state::BcDriveEntry* pDriveEntry, const Key& driveKey) {
			const auto& lastApprovedDataModificationId = pReplicatorEntry->drives().at(driveKey).LastApprovedDataModificationId;
			const auto& dataModificationIdIsValid = pReplicatorEntry->drives().at(driveKey).DataModificationIdIsValid;
			const auto& completedDataModifications = pDriveEntry->completedDataModifications();

			uint64_t approvableDownloadWork = 0;

			// Iterating over completed data modifications in reverse order (from newest to oldest).
			for (auto it = completedDataModifications.rbegin(); it != completedDataModifications.rend(); ++it) {

				// Exit the loop as soon as the most recent data modification approved by the replicator is reached. Don't account its size.
				// dataModificationIdIsValid prevents rare cases of premature exits when the drive had no approved data modifications when the replicator
				// joined it, but current data modification id happens to match the stored lastApprovedDataModification (zero hash by default).
				if (dataModificationIdIsValid && it->Id == lastApprovedDataModificationId)
					break;

				// If current data modification was approved (not cancelled), account its size.
				if (it->State == state::DataModificationState::Succeeded)
					approvableDownloadWork += it->UploadSize;
			}

			return approvableDownloadWork;
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
		manager.addTransactionSupport(CreateReplicatorOnboardingTransactionPlugin());
		manager.addTransactionSupport(CreateFinishDownloadTransactionPlugin());
		manager.addTransactionSupport(CreateDownloadPaymentTransactionPlugin());
		manager.addTransactionSupport(CreateStoragePaymentTransactionPlugin());
		manager.addTransactionSupport(CreateDataModificationSingleApprovalTransactionPlugin());
		manager.addTransactionSupport(CreateVerificationPaymentTransactionPlugin());
		manager.addTransactionSupport(CreateFinishDriveVerificationTransactionPlugin());

		manager.addAmountResolver([](const auto& cache, const auto& unresolved, auto& resolved) {
		  	switch (unresolved.Type) {
		  	case UnresolvedAmountType::DownloadWork: {
				const auto& pDownloadWork = castToUnresolvedData<model::DownloadWork>(unresolved.DataPtr);

				const auto& replicatorCache = cache.template sub<cache::ReplicatorCache>();
				const auto replicatorIter = replicatorCache.find(pDownloadWork->Replicator);
				const auto& pReplicatorEntry = replicatorIter.tryGet();
				const auto& driveCache = cache.template sub<cache::BcDriveCache>();
				const auto driveIter = driveCache.find(pDownloadWork->DriveKey);
				const auto& pDriveEntry = driveIter.tryGet();

				if (!pReplicatorEntry || !pDriveEntry)
					break;

				resolved = Amount(calculateApprovableDownloadWork(pReplicatorEntry, pDriveEntry, pDownloadWork->DriveKey) + pReplicatorEntry->drives().at(pDownloadWork->DriveKey).InitialDownloadWork);
				return true;
		  	}
		  	case UnresolvedAmountType::UploadWork: {
			  	const auto& pUploadWork = castToUnresolvedData<model::UploadWork>(unresolved.DataPtr);

				const auto& replicatorCache = cache.template sub<cache::ReplicatorCache>();
				const auto replicatorIter = replicatorCache.find(pUploadWork->Replicator);
				const auto& pReplicatorEntry = replicatorIter.tryGet();
				const auto& driveCache = cache.template sub<cache::BcDriveCache>();
				const auto driveIter = driveCache.find(pUploadWork->DriveKey);
				const auto& pDriveEntry = driveIter.tryGet();

				if (!pReplicatorEntry || !pDriveEntry)
					break;

				resolved = Amount(calculateApprovableDownloadWork(pReplicatorEntry, pDriveEntry, pUploadWork->DriveKey) * pUploadWork->Opinion);
			  	return true;
			}
			case UnresolvedAmountType::DownloadPayment: {
				const auto& pDownloadPayment = castToUnresolvedData<model::DownloadPayment>(unresolved.DataPtr);

				const auto& downloadChannelCache = cache.template sub<cache::DownloadChannelCache>();
				const auto downloadChannelIter = downloadChannelCache.find(pDownloadPayment->DownloadChannelId);
				const auto& pDownloadChannelEntry = downloadChannelIter.tryGet();

				if (!pDownloadChannelEntry)
					break;

				resolved = Amount(pDownloadPayment->DownloadSize * pDownloadChannelEntry->listOfPublicKeys().size());
				return true;
			}
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

		manager.addCacheSupport<cache::DownloadChannelCacheStorage>(
			std::make_unique<cache::DownloadChannelCache>(manager.cacheConfig(cache::DownloadChannelCache::Name), pConfigHolder));

		using DownloadChannelCacheHandlersService = CacheHandlers<cache::DownloadChannelCacheDescriptor>;
		DownloadChannelCacheHandlersService::Register<model::FacilityCode::DownloadChannel>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("DOWNLOAD CH C"), [&cache]() {
				return cache.sub<cache::DownloadChannelCache>().createView(cache.height())->size();
			});
		});

		auto pKeyCollector = std::make_shared<cache::ReplicatorKeyCollector>();
		manager.addCacheSupport(std::make_unique<cache::ReplicatorCacheSubCachePlugin>(
			manager.cacheConfig(cache::ReplicatorCache::Name), pKeyCollector, pConfigHolder));

		using ReplicatorCacheHandlersService = CacheHandlers<cache::ReplicatorCacheDescriptor>;
		ReplicatorCacheHandlersService::Register<model::FacilityCode::Replicator>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("REPLICATOR C"), [&cache]() {
				return cache.sub<cache::ReplicatorCache>().createView(cache.height())->size();
			});
		});

		manager.setStorageState(std::make_unique<state::CachedStorageState>(pKeyCollector));

		manager.addStatefulValidatorHook([pConfigHolder, &immutableConfig, pKeyCollector](auto& builder) {
		  	builder
				.add(validators::CreatePrepareDriveValidator(pKeyCollector))
				.add(validators::CreateDataModificationValidator())
				.add(validators::CreateDataModificationApprovalValidator())
				.add(validators::CreateDataModificationCancelValidator())
				.add(validators::CreateReplicatorOnboardingValidator())
				.add(validators::CreateFinishDownloadValidator())
				.add(validators::CreateDownloadPaymentValidator())
				.add(validators::CreateStoragePaymentValidator())
				.add(validators::CreateDataModificationSingleApprovalValidator())
		  		.add(validators::CreateVerificationPaymentValidator())
		  		.add(validators::CreateFinishDriveVerificationValidator());
		});

		manager.addObserverHook([pKeyCollector](auto& builder) {
			builder
				.add(observers::CreatePrepareDriveObserver(pKeyCollector))
				.add(observers::CreateDownloadChannelObserver())
				.add(observers::CreateDataModificationObserver())
				.add(observers::CreateDataModificationApprovalObserver())
				.add(observers::CreateDataModificationCancelObserver())
				.add(observers::CreateReplicatorOnboardingObserver())
				.add(observers::CreateDownloadPaymentObserver())
				.add(observers::CreateDataModificationSingleApprovalObserver())
				.add(observers::CreateFinishDriveVerificationObserver());
		});
	}
}}


extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterStorageSubsystem(manager);
}
