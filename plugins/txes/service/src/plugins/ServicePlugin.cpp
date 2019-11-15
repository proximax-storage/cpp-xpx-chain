/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/PluginManager.h"
#include "catapult/observers/ObserverUtils.h"
#include "ServicePlugin.h"
#include "src/cache/DriveCache.h"
#include "src/cache/DriveCacheStorage.h"
#include "src/model/ServiceNotifications.h"
#include "src/observers/Observers.h"
#include "src/plugins/DriveFileSystemTransactionPlugin.h"
#include "src/plugins/FilesDepositTransactionPlugin.h"
#include "src/plugins/JoinToDriveTransactionPlugin.h"
#include "src/plugins/PrepareDriveTransactionPlugin.h"
#include "src/plugins/EndDriveTransactionPlugin.h"
#include "src/plugins/DeleteRewardTransactionPlugin.h"
#include "src/plugins/StartDriveVerificationTransactionPlugin.h"
#include "src/plugins/EndDriveVerificationTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "src/utils/ServiceUtils.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterServiceSubsystem(PluginManager& manager) {
        const auto& pConfigHolder = manager.configHolder();
        const auto& immutableConfig = manager.immutableConfig();
        manager.addTransactionSupport(CreatePrepareDriveTransactionPlugin());
		manager.addTransactionSupport(CreateDriveFileSystemTransactionPlugin(pConfigHolder));
		manager.addTransactionSupport(CreateFilesDepositTransactionPlugin(pConfigHolder));
		manager.addTransactionSupport(CreateJoinToDriveTransactionPlugin(pConfigHolder));
		manager.addTransactionSupport(CreateEndDriveTransactionPlugin());
		manager.addTransactionSupport(CreateDeleteRewardTransactionPlugin());
		manager.addTransactionSupport(CreateStartDriveVerificationTransactionPlugin(pConfigHolder));
		manager.addTransactionSupport(CreateEndDriveVerificationTransactionPlugin(immutableConfig.NetworkIdentifier));

        manager.addAmountResolver([](const auto& cache, const auto& unresolved, auto& resolved) {
            const auto& driveCache = cache.template sub<cache::DriveCache>();
            switch (unresolved.Type) {
            	case UnresolvedAmountType::DriveDeposit: {
					const auto &driveKey = reinterpret_cast<model::DriveDeposit*>(unresolved.Data)->DriveKey;

					if (driveCache.contains(driveKey)) {
                        auto driveIter = driveCache.find(driveKey);
                        const auto& driveEntry = driveIter.get();
						resolved = utils::CalculateDriveDeposit(driveEntry);
						return true;
					}
					break;
				}
				case UnresolvedAmountType::FileDeposit: {
					auto fileDeposit = reinterpret_cast<model::FileDeposit*>(unresolved.Data);

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
					auto fileUpload = reinterpret_cast<model::FileUpload*>(unresolved.Data);

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

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
					.add(validators::CreatePrepareDriveArgumentsValidator())
					.add(validators::CreateDeleteRewardValidator())
					.add(validators::CreateServicePluginConfigValidator());
		});

		manager.addStatefulValidatorHook([pConfigHolder](auto& builder) {
			builder
					.add(validators::CreateDriveValidator())
					.add(validators::CreateExchangeValidator(pConfigHolder))
					.add(validators::CreateDrivePermittedOperationValidator())
					.add(validators::CreateFilesDepositValidator())
					.add(validators::CreateJoinToDriveValidator())
					.add(validators::CreatePrepareDrivePermissionValidator())
					.add(validators::CreateDriveFileSystemValidator())
					.add(validators::CreateEndDriveValidator(pConfigHolder))
					.add(validators::CreateRewardValidator())
					.add(validators::CreateMaxFilesOnDriveValidator(pConfigHolder))
					.add(validators::CreateStartDriveVerificationValidator(pConfigHolder))
					.add(validators::CreateEndDriveVerificationValidator(pConfigHolder));
		});

		auto storageMosaicId = immutableConfig.StorageMosaicId;
		manager.addObserverHook([pConfigHolder, storageMosaicId](auto& builder) {
			builder
					.add(observers::CreatePrepareDriveObserver())
					.add(observers::CreateDriveFileSystemObserver())
					.add(observers::CreateFilesDepositObserver())
					.add(observers::CreateJoinToDriveObserver())
                    .add(observers::CreateDriveVerificationPaymentObserver(storageMosaicId))
                    .add(observers::CreateStartBillingObserver(pConfigHolder))
                    .add(observers::CreateEndBillingObserver(pConfigHolder))
                    .add(observers::CreateEndDriveObserver(pConfigHolder))
                    .add(observers::CreateRewardObserver(pConfigHolder))
                    .add(observers::CreateDriveCacheBlockPruningObserver(pConfigHolder));
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterServiceSubsystem(manager);
}
