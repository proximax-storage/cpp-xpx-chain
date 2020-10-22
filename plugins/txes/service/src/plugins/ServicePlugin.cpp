/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/PluginManager.h"
#include "ServicePlugin.h"
#include "src/cache/DriveCache.h"
#include "src/cache/DriveCacheStorage.h"
#include "src/cache/DownloadCache.h"
#include "src/cache/DownloadCacheStorage.h"
#include "src/model/ServiceNotifications.h"
#include "src/observers/Observers.h"
#include "src/plugins/DriveFileSystemTransactionPlugin.h"
#include "src/plugins/FilesDepositTransactionPlugin.h"
#include "src/plugins/JoinToDriveTransactionPlugin.h"
#include "src/plugins/PrepareDriveTransactionPlugin.h"
#include "src/plugins/EndDriveTransactionPlugin.h"
#include "src/plugins/DriveFilesRewardTransactionPlugin.h"
#include "src/plugins/StartDriveVerificationTransactionPlugin.h"
#include "src/plugins/EndDriveVerificationTransactionPlugin.h"
#include "src/plugins/StartFileDownloadTransactionPlugin.h"
#include "src/plugins/EndFileDownloadTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "src/utils/ServiceUtils.h"
#include "catapult/observers/ObserverUtils.h"
#include "catapult/plugins/CacheHandlers.h"
#include "src/config/ServiceConfiguration.h"

namespace catapult { namespace plugins {

	namespace {
		void extractOwner(const cache::DriveCache::CacheReadOnlyType& cache, const Key& publicKey, model::PublicKeySet& result) {
			if (!cache.contains(publicKey))
				return;

			auto driveIter = cache.find(publicKey);
			const auto& driveEntry = driveIter.get();
			result.insert(driveEntry.owner());

			for (const auto& coowner : driveEntry.coowners())
				result.insert(coowner);
		}

		template<typename TUnresolvedData>
		const TUnresolvedData* castToUnresolvedData(const UnresolvedAmountData* pData) {
			if (!pData)
				CATAPULT_THROW_RUNTIME_ERROR("unresolved amount data pointer is null")

			auto pCast = dynamic_cast<const TUnresolvedData*>(pData);
			if (!pCast)
				CATAPULT_THROW_RUNTIME_ERROR("unresolved amount data pointer is of unexpected type")

			return pCast;
		}
	}

	void RegisterServiceSubsystem(PluginManager& manager) {

		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::ServiceConfiguration>();
		});


        const auto& pConfigHolder = manager.configHolder();
        const auto& immutableConfig = manager.immutableConfig();
        manager.addTransactionSupport(CreatePrepareDriveTransactionPlugin());
		manager.addTransactionSupport(CreateDriveFileSystemTransactionPlugin(pConfigHolder));
		manager.addTransactionSupport(CreateFilesDepositTransactionPlugin(pConfigHolder));
		manager.addTransactionSupport(CreateJoinToDriveTransactionPlugin(pConfigHolder));
		manager.addTransactionSupport(CreateEndDriveTransactionPlugin());
		manager.addTransactionSupport(CreateDriveFilesRewardTransactionPlugin());
		manager.addTransactionSupport(CreateStartDriveVerificationTransactionPlugin(pConfigHolder));
		manager.addTransactionSupport(CreateEndDriveVerificationTransactionPlugin(immutableConfig.NetworkIdentifier));
		manager.addTransactionSupport(CreateStartFileDownloadTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateEndFileDownloadTransactionPlugin(immutableConfig));

		manager.addPublicKeysExtractor([](const auto& cache, const auto& key) {
			const auto& driveCache = cache.template sub<cache::DriveCache>();
			auto result = model::PublicKeySet{ key };
			extractOwner(driveCache, key, result);

			return result;
		});

        manager.addAmountResolver([](const auto& cache, const auto& unresolved, auto& resolved) {
            const auto& driveCache = cache.template sub<cache::DriveCache>();
            switch (unresolved.Type) {
            	case UnresolvedAmountType::DriveDeposit: {
					const auto &driveKey = castToUnresolvedData<model::DriveDeposit>(unresolved.DataPtr)->DriveKey;

					if (driveCache.contains(driveKey)) {
                        auto driveIter = driveCache.find(driveKey);
                        const auto& driveEntry = driveIter.get();
						resolved = utils::CalculateDriveDeposit(driveEntry);
						return true;
					}
					break;
				}
				case UnresolvedAmountType::FileDeposit: {
					auto fileDeposit = castToUnresolvedData<model::FileDeposit>(unresolved.DataPtr);

					if (driveCache.contains(fileDeposit->DriveKey)) {
					    auto driveIter = driveCache.find(fileDeposit->DriveKey);
						const auto& driveEntry = driveIter.get();

						if (driveEntry.files().count(fileDeposit->FileHash)) {
							resolved = utils::CalculateFileDeposit(driveEntry, fileDeposit->FileHash);
							return true;
						}
					}
					break;
				}
				case UnresolvedAmountType::FileUpload: {
					auto fileUpload = castToUnresolvedData<model::FileUpload>(unresolved.DataPtr);

					if (driveCache.contains(fileUpload->DriveKey)) {
                        auto driveIter = driveCache.find(fileUpload->DriveKey);
                        const auto& driveEntry = driveIter.get();

						resolved = utils::CalculateFileUpload(driveEntry, fileUpload->FileSize);
						return true;
					}
					break;
				}
				default:
					break;
            }

            return false;
        });

		manager.addCacheSupport<cache::DriveCacheStorage>(
			std::make_unique<cache::DriveCache>(manager.cacheConfig(cache::DriveCache::Name), pConfigHolder));

		using DriveCacheHandlersService = CacheHandlers<cache::DriveCacheDescriptor>;
		DriveCacheHandlersService::Register<model::FacilityCode::Drive>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("DRIVE C"), [&cache]() {
				return cache.sub<cache::DriveCache>().createView(cache.height())->size();
			});
		});

		manager.addCacheSupport<cache::DownloadCacheStorage>(
			std::make_unique<cache::DownloadCache>(manager.cacheConfig(cache::DownloadCache::Name), pConfigHolder));

		using DownloadCacheHandlersService = CacheHandlers<cache::DownloadCacheDescriptor>;
		DownloadCacheHandlersService::Register<model::FacilityCode::Download>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("DOWNLOAD C"), [&cache]() {
				return cache.sub<cache::DownloadCache>().createView(cache.height())->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
					.add(validators::CreatePrepareDriveArgumentsV1Validator())
					.add(validators::CreatePrepareDriveArgumentsV2Validator())
					.add(validators::CreateServicePluginConfigValidator())
					.add(validators::CreateFailedBlockHashesValidator());
		});

		manager.addStatefulValidatorHook([pConfigHolder, &immutableConfig](auto& builder) {
			builder
					.add(validators::CreateDriveValidator())
					.add(validators::CreateExchangeV1Validator())
					.add(validators::CreateExchangeV2Validator())
					.add(validators::CreateDrivePermittedOperationValidator())
                    .add(validators::CreateDriveFilesRewardValidator(immutableConfig.StreamingMosaicId))
					.add(validators::CreateFilesDepositValidator())
					.add(validators::CreateJoinToDriveValidator())
					.add(validators::CreatePrepareDrivePermissionV1Validator())
					.add(validators::CreatePrepareDrivePermissionV2Validator())
					.add(validators::CreateDriveFileSystemValidator())
					.add(validators::CreateEndDriveValidator())
					.add(validators::CreateMaxFilesOnDriveValidator())
					.add(validators::CreateStartDriveVerificationValidator())
					.add(validators::CreateEndDriveVerificationValidator())
					.add(validators::CreateStartFileDownloadValidator())
					.add(validators::CreateEndFileDownloadValidator());
		});

		manager.addObserverHook([pConfigHolder, &immutableConfig](auto& builder) {
			builder
					.add(observers::CreatePrepareDriveV1Observer())
					.add(observers::CreatePrepareDriveV2Observer())
					.add(observers::CreateDriveFileSystemObserver(immutableConfig.StreamingMosaicId))
					.add(observers::CreateFilesDepositObserver())
					.add(observers::CreateJoinToDriveObserver())
                    .add(observers::CreateDriveVerificationPaymentObserver(immutableConfig.StorageMosaicId))
                    .add(observers::CreateStartBillingObserver(immutableConfig.StorageMosaicId))
                    .add(observers::CreateEndBillingObserver(immutableConfig.StorageMosaicId))
                    .add(observers::CreateEndDriveObserver(immutableConfig))
                    .add(observers::CreateDriveFilesRewardObserver(immutableConfig))
                    .add(observers::CreateDriveCacheBlockPruningObserver())
                    .add(observers::CreateStartFileDownloadObserver())
                    .add(observers::CreateEndFileDownloadObserver())
					.add(observers::CreateCacheBlockTouchObserver<cache::DownloadCache>("DownloadCache", model::Receipt_Type_Drive_Download_Expired))
					.add(observers::CreateCacheBlockPruningObserver<cache::DownloadCache>("DownloadCache", 1));
		});
	}
}}


extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterServiceSubsystem(manager);
}
