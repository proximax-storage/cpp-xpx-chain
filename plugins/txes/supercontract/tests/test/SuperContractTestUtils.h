/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/model/EntityBody.h"
#include "catapult/model/Mosaic.h"
#include "catapult/utils/MemoryUtils.h"
#include "src/cache/SuperContractCache.h"
#include "src/cache/SuperContractCacheStorage.h"
#include "src/model/SuperContractEntityType.h"
#include "plugins/txes/operation/src/cache/OperationCache.h"
#include "plugins/txes/operation/src/cache/OperationCacheStorage.h"
#include "plugins/txes/operation/tests/test/OperationTestUtils.h"
#include "plugins/txes/service/src/cache/DriveCache.h"
#include "plugins/txes/service/src/cache/DriveCacheStorage.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/nodeps/Random.h"

namespace catapult { namespace test {

	/// Creates test drive entry.
	state::SuperContractEntry CreateSuperContractEntry(
		Key key = test::GenerateRandomByteArray<Key>(),
		state::SuperContractState state = static_cast<state::SuperContractState>(test::RandomByte()),
		Key owner = test::GenerateRandomByteArray<Key>(),
		Height start = test::GenerateRandomValue<Height>(),
		Height end = test::GenerateRandomValue<Height>(),
		VmVersion vmVersion = test::GenerateRandomValue<VmVersion>(),
		Key mainDriveKey = test::GenerateRandomByteArray<Key>(),
		Hash256 fileHash = test::GenerateRandomByteArray<Hash256>(),
		uint16_t executionCount = test::Random16());

	/// Verifies that \a entry1 is equivalent to \a entry2.
	void AssertEqualSuperContractData(const state::SuperContractEntry& entry1, const state::SuperContractEntry& entry2);

	/// Cache factory for creating a catapult cache composed of operation cache and core caches.
	struct SuperContractCacheFactory {
	private:
		static auto CreateSubCachesWithSuperContractCache(std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder) {
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cache::OperationCache::Id + 1);
			subCaches[cache::DriveCache::Id] = MakeSubCachePlugin<cache::DriveCache, cache::DriveCacheStorage>(pConfigHolder);
			subCaches[cache::OperationCache::Id] = MakeSubCachePlugin<cache::OperationCache, cache::OperationCacheStorage>(pConfigHolder);
			subCaches[cache::SuperContractCache::Id] = MakeSubCachePlugin<cache::SuperContractCache, cache::SuperContractCacheStorage>(pConfigHolder);
			return subCaches;
		}

	public:
		/// Creates an empty catapult cache around default configuration.
		static cache::CatapultCache Create() {
			auto pConfigHolder = config::CreateMockConfigurationHolder();
			auto subCaches = CreateSubCachesWithSuperContractCache(pConfigHolder);
			CoreSystemCacheFactory::CreateSubCaches(pConfigHolder->Config(), subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const config::BlockchainConfiguration& config) {
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			auto subCaches = CreateSubCachesWithSuperContractCache(pConfigHolder);
			CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}
	};

    /// Creates a deploy transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateDeployTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_Deploy);
		pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
		pTransaction->Owner = test::GenerateRandomByteArray<Key>();
		pTransaction->FileHash = test::GenerateRandomByteArray<Hash256>();
		pTransaction->VmVersion = test::GenerateRandomValue<VmVersion>();

        return pTransaction;
    }

    /// Creates a start execute transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateStartExecuteTransaction(size_t numMosaics, size_t functionSize, size_t dataSize) {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_StartExecute,
        	numMosaics * sizeof(model::UnresolvedMosaic) + functionSize + dataSize);
		pTransaction->SuperContract = test::GenerateRandomByteArray<Key>();
		pTransaction->MosaicCount = numMosaics;
		pTransaction->FunctionSize = functionSize;
		pTransaction->DataSize = dataSize;
		GenerateMosaics(pTransaction.get(), numMosaics, functionSize);

        return pTransaction;
    }

    /// Creates an end execute transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateEndExecuteTransaction(size_t numMosaics) {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_EndExecute, numMosaics * sizeof(model::UnresolvedMosaic));
		pTransaction->OperationToken = test::GenerateRandomByteArray<Hash256>();
		pTransaction->Result = test::Random16();
		pTransaction->MosaicCount = numMosaics;
		GenerateMosaics(pTransaction.get(), numMosaics);

        return pTransaction;
    }

    /// Creates an upload file transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateUploadFileTransaction(size_t numAddActions, size_t numRemoveActions) {
		auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_UploadFile,
			numAddActions * sizeof(model::AddAction) + numRemoveActions * sizeof(model::RemoveAction));
		pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
		pTransaction->RootHash = test::GenerateRandomByteArray<Hash256>();
		pTransaction->XorRootHash = test::GenerateRandomByteArray<Hash256>();
		pTransaction->AddActionsCount = numAddActions;
		pTransaction->RemoveActionsCount = numRemoveActions;

        auto* pData = reinterpret_cast<uint8_t*>(pTransaction.get() + 1);
		for (auto i = 0u; i < numAddActions; ++i) {
			model::AddAction addAction{{test::GenerateRandomByteArray<Hash256>()}, test::Random()};
            memcpy(pData, static_cast<const void*>(&addAction), sizeof(model::AddAction));
            pData += sizeof(model::AddAction);
        }

		for (auto i = 0u; i < numRemoveActions; ++i) {
			model::RemoveAction removeAction{{{test::GenerateRandomByteArray<Hash256>()}, test::Random()}};
            memcpy(pData, static_cast<const void*>(&removeAction), sizeof(model::RemoveAction));
            pData += sizeof(model::RemoveAction);
        }

        return pTransaction;
    }

    /// Creates a deactivate transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateDeactivateTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_Deactivate);
		pTransaction->SuperContract = test::GenerateRandomByteArray<Key>();

        return pTransaction;
    }
}}
