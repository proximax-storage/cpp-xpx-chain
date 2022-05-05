/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StoragePlugin.h"
#include <src/cache/QueueCacheStorage.h>
#include "src/cache/BcDriveCacheStorage.h"
#include "src/cache/DownloadChannelCacheStorage.h"
#include "src/cache/ReplicatorCacheSubCachePlugin.h"
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
#include "src/state/StorageStateImpl.h"
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
					approvableDownloadWork += it->ActualUploadSizeMegabytes;
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

		manager.addAmountResolver([](const auto& cache, const auto& unresolved, auto& resolved) {
			switch (unresolved.Type) {
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

		manager.addCacheSupport<cache::QueueCacheStorage>(
				std::make_unique<cache::QueueCache>(manager.cacheConfig(cache::QueueCache::Name), pConfigHolder));

		using QueueCacheHandlersService = CacheHandlers<cache::QueueCacheDescriptor>;
		QueueCacheHandlersService::Register<model::FacilityCode::Queue>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("QUEUE C"), [&cache]() {
				return cache.sub<cache::QueueCache>().createView(cache.height())->size();
			});
		});

		auto pReplicatorKeyCollector = std::make_shared<cache::ReplicatorKeyCollector>();
		manager.addCacheSupport(std::make_unique<cache::ReplicatorCacheSubCachePlugin>(
			manager.cacheConfig(cache::ReplicatorCache::Name), pReplicatorKeyCollector, pConfigHolder));

		using ReplicatorCacheHandlersService = CacheHandlers<cache::ReplicatorCacheDescriptor>;
		ReplicatorCacheHandlersService::Register<model::FacilityCode::Replicator>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("REPLICATOR C"), [&cache]() {
				return cache.sub<cache::ReplicatorCache>().createView(cache.height())->size();
			});
		});

		auto pStorageState = std::make_shared<state::StorageStateImpl>(pReplicatorKeyCollector);
		manager.setStorageState(pStorageState);

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateStoragePluginConfigValidator());
		});

		using DrivePriority = std::pair<Key, double>;
		using DriveQueue = std::priority_queue<DrivePriority, std::vector<DrivePriority>, utils::DriveQueueComparator>;
		auto pDriveQueue = std::make_shared<DriveQueue>();

		manager.addStatefulValidatorHook([pConfigHolder, &immutableConfig, pReplicatorKeyCollector](auto& builder) {
		  	builder
				.add(validators::CreatePrepareDriveValidator(pReplicatorKeyCollector))
				.add(validators::CreateDataModificationValidator())
				.add(validators::CreateDataModificationApprovalValidator())
				.add(validators::CreateDataModificationApprovalDownloadWorkValidator())
				.add(validators::CreateDataModificationApprovalUploadWorkValidator())
				.add(validators::CreateDataModificationApprovalRefundValidator())
				.add(validators::CreateDataModificationCancelValidator())
				.add(validators::CreateDriveClosureValidator())
				.add(validators::CreateReplicatorOnboardingValidator())
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
				.add(validators::CreateEndDriveVerificationValidator());
		});

		manager.addObserverHook([pReplicatorKeyCollector, &state = *pStorageState, pDriveQueue](auto& builder) {
			builder
				.add(observers::CreatePrepareDriveObserver(pReplicatorKeyCollector, pDriveQueue))
				.add(observers::CreateDownloadChannelObserver())
				.add(observers::CreateDataModificationObserver(pReplicatorKeyCollector, pDriveQueue))
				.add(observers::CreateDataModificationApprovalObserver())
				.add(observers::CreateDataModificationApprovalDownloadWorkObserver())
				.add(observers::CreateDataModificationApprovalUploadWorkObserver())
				.add(observers::CreateDataModificationApprovalRefundObserver())
				.add(observers::CreateDataModificationCancelObserver())
				.add(observers::CreateDriveClosureObserver(pDriveQueue))
				.add(observers::CreateReplicatorOnboardingObserver(pDriveQueue))
				.add(observers::CreateReplicatorOffboardingObserver(pDriveQueue))
				.add(observers::CreateDownloadPaymentObserver())
				.add(observers::CreateDataModificationSingleApprovalObserver())
				.add(observers::CreateDownloadApprovalObserver())
				.add(observers::CreateFinishDownloadObserver())
				.add(observers::CreateDownloadApprovalPaymentObserver())
				.add(observers::CreateDownloadChannelRefundObserver())
				.add(observers::CreateStreamStartObserver())
				.add(observers::CreateStreamFinishObserver())
				.add(observers::CreateStreamPaymentObserver())
				.add(observers::CreateStartDriveVerificationObserver(state))
				.add(observers::CreateEndDriveVerificationObserver(pReplicatorKeyCollector, pDriveQueue))
				.add(observers::CreatePeriodicStoragePaymentObserver(pDriveQueue))
				.add(observers::CreatePeriodicDownloadChannelPaymentObserver());
		});
	}
}}


extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterStorageSubsystem(manager);
}
