/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/model/EntityBody.h"
#include "catapult/model/Mosaic.h"
#include "catapult/utils/MemoryUtils.h"
#include "src/cache/OperationCache.h"
#include "src/model/OperationEntityType.h"
#include "plugins/txes/lock_shared/tests/test/LockInfoCacheTestUtils.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/ResolverTestUtils.h"
#include "tests/test/nodeps/Random.h"

namespace catapult { namespace test {

	/// Basic traits for an operation entry.
	struct BasicOperationTestTraits : public cache::OperationCacheDescriptor {
		using cache::OperationCacheDescriptor::ValueType;

		static constexpr auto ToKey = cache::OperationCacheDescriptor::GetKeyFromValue;

		/// Creates an operation entry with given \a height and \a status.
		static ValueType CreateLockInfo(Height height, state::LockStatus status = state::LockStatus::Unused);

		/// Creates a random operation entry.
		static ValueType CreateLockInfo();

		/// Sets the \a key of the \a lockInfo.
		static void SetKey(ValueType& lockInfo, const KeyType& key);

		/// Asserts that the operation entrys \a lhs and \a rhs are equal.
		static void AssertEqual(const ValueType& lhs, const ValueType& rhs);
	};

	/// Creates test drive entry.
	state::OperationEntry CreateOperationEntry(
		Hash256 operationToken = test::GenerateRandomByteArray<Hash256>(),
		Height height = test::GenerateRandomValue<Height>(),
		state::LockStatus status = state::LockStatus::Unused,
		Key initiator = test::GenerateRandomByteArray<Key>(),
		uint16_t mosaicCount = 2,
		uint16_t executorCount = 2,
		uint16_t transactionHashCount = 2);

	/// Verifies that \a entry1 is equivalent to \a entry2.
	void AssertEqualOperationData(const state::OperationEntry& entry1, const state::OperationEntry& entry2);

	/// Cache factory for creating a catapult cache composed of operation cache and core caches.
	struct OperationCacheFactory {
	private:
		static auto CreateSubCachesWithOperationCache(const config::BlockchainConfiguration& config) {
			auto cacheId = cache::OperationCache::Id;
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cacheId + 1);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			subCaches[cacheId] = MakeSubCachePlugin<cache::OperationCache, cache::OperationCacheStorage>(pConfigHolder);
			return subCaches;
		}

	public:
		/// Creates an empty catapult cache around default configuration.
		static cache::CatapultCache Create() {
			return Create(test::MutableBlockchainConfiguration().ToConst());
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const config::BlockchainConfiguration& config) {
			auto subCaches = CreateSubCachesWithOperationCache(config);
			CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}
	};

    /// Creates a transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateTransaction(model::EntityType type, size_t additionalSize = 0) {
        uint32_t entitySize = sizeof(TTransaction) + additionalSize;
        auto pTransaction = utils::MakeUniqueWithSize<TTransaction>(entitySize);
		pTransaction->Signer = test::GenerateRandomByteArray<Key>();
		pTransaction->Version = model::MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);
        pTransaction->Type = type;
        pTransaction->Size = entitySize;

        return pTransaction;
    }

    template<typename TTransaction>
	void GenerateMosaics(TTransaction* pTransaction, size_t numMosaics) {
		auto* pData = reinterpret_cast<uint8_t*>(pTransaction + 1);
		for (auto i = 0u; i < numMosaics; ++i) {
			model::UnresolvedMosaic mosaic{test::UnresolveXor(MosaicId(i + 1)), Amount((i + 1) * 10)};
			memcpy(pData, static_cast<const void*>(&mosaic), sizeof(model::UnresolvedMosaic));
			pData += sizeof(model::UnresolvedMosaic);
		}
    }

    /// Creates an operation identify transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateOperationIdentifyTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_OperationIdentify);
		pTransaction->OperationToken = test::GenerateRandomByteArray<Hash256>();

        return pTransaction;
    }

    /// Creates a start operation transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateStartOperationTransaction(size_t numMosaics, size_t numExecutors) {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_StartOperation,
        	numMosaics * sizeof(model::UnresolvedMosaic) + numExecutors * Key_Size);
		pTransaction->Duration = test::GenerateRandomValue<BlockDuration>();
		pTransaction->MosaicCount = numMosaics;
		pTransaction->ExecutorCount = numExecutors;
		GenerateMosaics(pTransaction.get(), numMosaics);

        auto* pData = reinterpret_cast<uint8_t*>(pTransaction.get() + 1) + numMosaics * sizeof(model::UnresolvedMosaic);
		for (auto i = 0u; i < numExecutors; ++i) {
			auto executor = test::GenerateRandomByteArray<Key>();
            memcpy(pData, static_cast<const void*>(&executor), Key_Size);
            pData += Key_Size;
        }

        return pTransaction;
    }

    /// Creates an end operation transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateEndOperationTransaction(size_t numMosaics) {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_EndOperation, numMosaics * sizeof(model::UnresolvedMosaic));
		pTransaction->OperationToken = test::GenerateRandomByteArray<Hash256>();
		pTransaction->Result = test::Random16();
		pTransaction->MosaicCount = numMosaics;
		GenerateMosaics(pTransaction.get(), numMosaics);

        return pTransaction;
    }
}}
