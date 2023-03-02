/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/cache/SuperContractCache.h"
#include "src/cache/SuperContractCacheStorage.h"
#include "src/cache/DriveContractCache.h"
#include "src/cache/DriveContractCacheStorage.h"
#include "src/model/SuperContractEntityType.h"
#include "src/model/EndBatchExecutionModel.h"
#include "src/model/DeployContractTransaction.h"
#include "src/state/DriveContractEntry.h"
#include "src/state/SuperContractEntry.h"
#include "catapult/model/EntityPtr.h"
#include "catapult/model/EntityBody.h"
#include "catapult/model/Mosaic.h"
#include "catapult/model/SupercontractModel.h"
#include "catapult/utils/MemoryUtils.h"
#include "plugins/txes/storage/src/state/DriveStateBrowserImpl.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/nodeps/Random.h"
#include "tests/TestHarness.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"

namespace catapult { namespace test {

	// Create serializer supercontract entry
	state::SuperContractEntry CreateSuperContractEntrySerializer(Key key = test::GenerateRandomByteArray<Key>());

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

	/// Creates a automatic executions payment transaction.
	template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateAutomaticExecutionsPaymentTransaction() {
		auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_AutomaticExecutionsPaymentTransaction);
		pTransaction->ContractKey = test::GenerateRandomByteArray<Key>();
		pTransaction->AutomaticExecutionsNumber = test::Random32();
		return pTransaction;
	}

	/// Creates a deploy contract transaction.
	template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateDeployContractTransaction() {
		// dynamic data size
		uint16_t fileNamePtrSize = test::Random16();
		uint16_t functionNamePtrSize = test::Random16();
		uint16_t actualArgumentsPtrSize = test::Random16();
		uint8_t servicePaymentsPtrSize = 3;
		uint16_t automaticExecutionFileNamePtrSize = test::Random16();
		uint16_t automaticExecutionFunctionNamePtrSize = test::Random16();
		uint64_t additionalSize = fileNamePtrSize +
							  functionNamePtrSize +
							  actualArgumentsPtrSize +
							  servicePaymentsPtrSize * sizeof(model::UnresolvedMosaic) +
							  automaticExecutionFileNamePtrSize +
							  automaticExecutionFunctionNamePtrSize;

		auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_DeployContractTransaction, additionalSize);
		pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
		pTransaction->FileNameSize = fileNamePtrSize;
		pTransaction->FunctionNameSize = functionNamePtrSize;
		pTransaction->ActualArgumentsSize = actualArgumentsPtrSize;
		pTransaction->ServicePaymentsCount = servicePaymentsPtrSize;
		pTransaction->ExecutionCallPayment = Amount(test::Random());
		pTransaction->DownloadCallPayment = Amount(test::Random());
		pTransaction->AutomaticExecutionFileNameSize = automaticExecutionFileNamePtrSize;
		pTransaction->AutomaticExecutionFunctionNameSize = automaticExecutionFunctionNamePtrSize;
		pTransaction->AutomaticExecutionCallPayment = Amount(test::Random());
		pTransaction->AutomaticDownloadCallPayment = Amount(test::Random());
		pTransaction->AutomaticExecutionsNumber = test::Random32();
		pTransaction->Assignee = test::GenerateRandomByteArray<Key>();
		return pTransaction;
	}

	/// Creates a end batch execution single transaction.
	template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateEndBatchExecutionSingleTransaction() {
		auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_EndBatchExecutionSingleTransaction);
		pTransaction->ContractKey = test::GenerateRandomByteArray<Key>();
		pTransaction->BatchId = test::Random();
		pTransaction->ProofOfExecution = model::RawProofOfExecution();
		return pTransaction;
	}

	/// Creates a synchronization single transaction.
	template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateSynchronizationSingleTransaction() {
		auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_SynchronizationSingleTransaction);
		pTransaction->ContractKey = test::GenerateRandomByteArray<Key>();
		pTransaction->BatchId = test::Random();
		return pTransaction;
	}

	/// Creates a successful end batch execution transaction.
	template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateSuccessfulEndBatchExecutionTransaction() {
		// dynamic data size
		uint16_t CosignersNumber = 5;
		uint16_t CallsNumber = 5;
		uint64_t additionalSize = CosignersNumber * Key_Size +
								  CosignersNumber * Signature_Size +
								  CosignersNumber * sizeof(model::RawProofOfExecution) +
								  CallsNumber * sizeof(model::ExtendedCallDigest) +
								  static_cast<uint64_t>(CosignersNumber) * static_cast<uint64_t>(CallsNumber) * sizeof(model::CallPayment);

		auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_SuccessfulEndBatchExecutionTransaction, additionalSize);
		pTransaction->ContractKey = test::GenerateRandomByteArray<Key>();
		pTransaction->BatchId = test::Random();
		pTransaction->StorageHash = test::GenerateRandomByteArray<Hash256>();
		pTransaction->UsedSizeBytes = test::Random();
		pTransaction->MetaFilesSizeBytes = test::Random();
		pTransaction->ProofOfExecutionVerificationInformation = test::GenerateRandomByteArray<std::array<uint8_t, 32>>();
		pTransaction->AutomaticExecutionsNextBlockToCheck = Height(test::Random());
		pTransaction->CosignersNumber = CosignersNumber;
		pTransaction->CallsNumber = CallsNumber;
		return pTransaction;
	}

	/// Creates a unsuccessful end batch execution transaction.
	template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateUnsuccessfulEndBatchExecutionTransaction() {
		// dynamic data size
		uint16_t CosignersNumber = 5;
		uint16_t CallsNumber = 5;
		uint64_t additionalSize = CosignersNumber * Key_Size +
								  CosignersNumber * Signature_Size +
								  CosignersNumber * sizeof(model::RawProofOfExecution) +
								  CallsNumber * sizeof(model::ShortCallDigest) +
								  CosignersNumber * CallsNumber * sizeof(model::CallPayment);

		auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_UnsuccessfulEndBatchExecutionTransaction, additionalSize);
		pTransaction->ContractKey = test::GenerateRandomByteArray<Key>();
		pTransaction->BatchId = test::Random();
		pTransaction->AutomaticExecutionsNextBlockToCheck = Height(test::Random());
		pTransaction->CosignersNumber = CosignersNumber;
		pTransaction->CallsNumber = CallsNumber;
		return pTransaction;
	}

	/// Creates a manual call transaction.
	template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateManualCallTransaction() {
		// dynamic data size
		uint16_t fileNamePtrSize = test::Random16();
		uint16_t functionNamePtrSize = test::Random16();
		uint16_t actualArgumentsPtrSize = test::Random16();
		uint8_t servicePaymentsPtrSize = 5;
		uint64_t additionalSize = fileNamePtrSize +
								  functionNamePtrSize +
								  actualArgumentsPtrSize +
								  servicePaymentsPtrSize * sizeof(model::UnresolvedMosaic);

		auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_ManualCallTransaction, additionalSize);
		pTransaction->ContractKey = test::GenerateRandomByteArray<Key>();
		pTransaction->FileNameSize = fileNamePtrSize;
		pTransaction->FunctionNameSize = functionNamePtrSize;
		pTransaction->ActualArgumentsSize = actualArgumentsPtrSize;
		pTransaction->ServicePaymentsCount = servicePaymentsPtrSize;
		pTransaction->ExecutionCallPayment = Amount(test::Random());
		pTransaction->DownloadCallPayment = Amount(test::Random());
		return pTransaction;
	}

	/// Creates test sc entry.
	state::SuperContractEntry CreateSuperContractEntry(
		Key superContractKey = test::GenerateRandomByteArray<Key>(),
		Key driveKey = test::GenerateRandomByteArray<Key>(),
		Key superContractOwnerKey = test::GenerateRandomByteArray<Key>(),
		Key executionPaymentKey = test::GenerateRandomByteArray<Key>(),
		Key creatorKey = test::GenerateRandomByteArray<Key>(),
		Hash256 deploymentBaseModificationId  = test::GenerateRandomByteArray<Hash256>());

	state::DriveContractEntry CreateDriveContractEntry(
			Key drive = test::GenerateRandomByteArray<Key>(),
			Key contract = test::GenerateRandomByteArray<Key>());

	void AssertEqualAutomaticExecutionsInfo(const state::AutomaticExecutionsInfo& entry1, const state::AutomaticExecutionsInfo& entry2);
	void AssertEqualServicePayments(const std::vector<state::ServicePayment>& entry1, const std::vector<state::ServicePayment>& entry2);
	void AssertEqualRequestedCalls(const std::deque<state::ContractCall>& entry1, const std::deque<state::ContractCall>& entry2);
	void AssertEqualSuperContractData(const state::SuperContractEntry& entry1, const state::SuperContractEntry& entry2);
	void AssertEqualDriveContract(const state::DriveContractEntry& entry1, const state::DriveContractEntry& entry2);

	/// Cache factory for creating a catapult cache composed of operation cache and core caches.
	struct SuperContractCacheFactory {
	private:
		static auto CreateSubCachesWithSuperContractCache(std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder) {
			std::vector<size_t> cacheIds = {
				cache::SuperContractCache::Id,
				cache::DriveContractCache::Id,
			};

			auto maxId = *std::max_element(cacheIds.begin(), cacheIds.end());
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(maxId + 1);

			subCaches[cache::SuperContractCache::Id] = MakeSubCachePlugin<cache::SuperContractCache, cache::SuperContractCacheStorage>(pConfigHolder);
			subCaches[cache::DriveContractCache::Id] = MakeSubCachePlugin<cache::DriveContractCache, cache::DriveContractCacheStorage>(pConfigHolder);

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

	class DriveStateBrowserImpl : public state::DriveStateBrowser {
	public:
		uint16_t getOrderedReplicatorsCount(
				const cache::ReadOnlyCatapultCache& cache,
				const Key& driveKey) const override;

		std::set<Key> getReplicators(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const override;

		std::set<Key> getDrives(const cache::ReadOnlyCatapultCache& cache, const Key& replicatorKey) const override;

		Hash256 getDriveState(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const override;

		Hash256 getLastModificationId(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const override;
	};
}}