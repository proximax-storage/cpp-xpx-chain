/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/PluginManager.h"
#include "ServicePlugin.h"
#include "src/cache/DriveCache.h"
#include "src/cache/DriveCacheStorage.h"
#include "src/cache/FileCache.h"
#include "src/cache/FileCacheStorage.h"
#include "src/observers/Observers.h"
#include "src/plugins/ServiceTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterServiceSubsystem(PluginManager& manager) {
		manager.addTransactionSupport(CreateServiceTransactionPlugin(manager.immutableConfig()));

		const auto& pConfigHolder = manager.configHolder();
		manager.addCacheSupport<cache::DriveCacheStorage>(
			std::make_unique<cache::DriveCache>(manager.cacheConfig(cache::DriveCache::Name), pConfigHolder));

		using DriveCacheHandlersService = CacheHandlers<cache::DriveCacheDescriptor>;
		DriveCacheHandlersService::Register<model::FacilityCode::Drive>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("DRIVE C"), [&cache]() {
				return cache.sub<cache::DriveCache>().createView(cache.height())->size();
			});
		});

		manager.addCacheSupport<cache::FileCacheStorage>(
			std::make_unique<cache::FileCache>(manager.cacheConfig(cache::FileCache::Name), pConfigHolder));

		using FileCacheHandlersService = CacheHandlers<cache::FileCacheDescriptor>;
		FileCacheHandlersService::Register<model::FacilityCode::File>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("FILE C"), [&cache]() {
				return cache.sub<cache::FileCache>().createView(cache.height())->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
					.add(validators::CreateTransferMosaicsValidator())
					.add(validators::CreateMosaicIdValidator())
					.add(validators::CreateDriveProlongationValidator())
					.add(validators::CreateServicePluginConfigValidator());
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
					.add(validators::CreateDriveValidator())
					.add(validators::CreateReplicatorValidator())
					.add(validators::CreatePrepareDriveValidator())
					.add(validators::CreateDriveDepositValidator())
					.add(validators::CreateDriveDepositReturnValidator())
					.add(validators::CreateFileDepositReturnValidator())
					.add(validators::CreateCreateDirectoryValidator())
					.add(validators::CreateRemoveDirectoryValidator())
					.add(validators::CreateUploadFileValidator())
					.add(validators::CreateDownloadFileValidator())
					.add(validators::CreateDeleteFileValidator())
					.add(validators::CreateMoveFileValidator())
					.add(validators::CreateCopyFileValidator());
		});

		manager.addObserverHook([pConfigHolder = manager.configHolder()](auto& builder) {
			builder
					.add(observers::CreatePrepareDriveObserver())
					.add(observers::CreateDriveProlongationObserver())
					.add(observers::CreateDriveDepositObserver(pConfigHolder))
					.add(observers::CreateFileDepositObserver())
					.add(observers::CreateDriveVerificationObserver())
					.add(observers::CreateCreateDirectoryObserver())
					.add(observers::CreateRemoveDirectoryObserver())
					.add(observers::CreateUploadFileObserver())
					.add(observers::CreateDeleteFileObserver())
					.add(observers::CreateMoveFileObserver())
					.add(observers::CreateCopyFileObserver());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterServiceSubsystem(manager);
}
