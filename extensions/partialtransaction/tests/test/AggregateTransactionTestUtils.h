/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "plugins/txes/aggregate/src/model/AggregateTransaction.h"
#include "catapult/utils/Hashers.h"
#include "tests/test/core/EntityTestUtils.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/mocks/MockTransaction.h"
#include <unordered_map>

namespace catapult { namespace test {


	/// Creates an aggregate transaction with \a numCosignatures cosignatures, \a numTransactions transactions and populates \a cosigners with the KeyPairs of \a cosignersAccountVersion version, used as transaction signers.
	/// \a cosignersAccountVersion size must be equal to numCosignatures or numTransactions, whichever is bigger.
	template<typename TDescriptor>
	model::UniqueEntityPtr<model::AggregateTransaction<TDescriptor>> CreateRandomAggregateTransactionWithCosignatures(uint32_t numCosignatures, uint32_t numTransactions, std::vector<crypto::KeyPair>& cosigners, const std::vector<uint32_t>& cosignersAccountVersion, bool signerIsCosigner) {
		if(signerIsCosigner)
			assert(!(!numCosignatures && !numTransactions));

		auto totalKeys = numCosignatures > numTransactions ? numCosignatures : numTransactions;
		for (auto i = 0u; i < totalKeys; ++i) {
			cosigners.push_back(test::GenerateKeyPair(cosignersAccountVersion[i]));
		}

		uint32_t payloadSize = numTransactions * sizeof(mocks::EmbeddedMockTransaction);
		uint32_t size = sizeof(model::AggregateTransaction<TDescriptor>) + payloadSize + (numCosignatures-(signerIsCosigner && numCosignatures > 0? 1 : 0)) * sizeof(typename TDescriptor::CosignatureType);

		auto pTransaction = utils::MakeUniqueWithSize<model::AggregateTransaction<TDescriptor>>(size);
		FillWithRandomData({ reinterpret_cast<uint8_t*>(pTransaction.get()), size });

		// If signerIsCosigner is enabled, the aggregate signer will also sign one of the embedded transactions. If no transactions or cosignatures exist, the signer can be anything.
		if(!numCosignatures && !numTransactions || !signerIsCosigner)
			FillWithRandomData(pTransaction->Signer);
		else
			pTransaction->Signer = cosigners[0].publicKey();

		if constexpr(std::is_same_v<TDescriptor, model::AggregateTransactionRawDescriptor>)
		{
			pTransaction->Type = model::Entity_Type_Aggregate_Bonded_V1;
			pTransaction->Version = MakeVersion(model::NetworkIdentifier::Mijin_Test, 3);
		}
		else
		{
			pTransaction->Type = model::Entity_Type_Aggregate_Bonded_V2;
			pTransaction->Version = MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);
		}

		pTransaction->Size = size;
		pTransaction->MaxFee = Amount(UINT16_MAX);
		pTransaction->PayloadSize = payloadSize;
		pTransaction->TransactionsPtr()->Size = payloadSize;

		auto* pSubTransaction = static_cast<mocks::EmbeddedMockTransaction*>(pTransaction->TransactionsPtr());
		for (auto i = 0u; i < numTransactions; ++i) {
			pSubTransaction->Size = sizeof(mocks::EmbeddedMockTransaction);

			// Set network identifier for successful validation
			pSubTransaction->Version = MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);
			pSubTransaction->Data.Size = 0;
			pSubTransaction->SetSignatureDerivationScheme(i < cosignersAccountVersion.size() ? utils::AccountVersionFeatureResolver::KeyDerivationScheme(cosignersAccountVersion[i]) : utils::AccountVersionFeatureResolver::KeyDerivationScheme(cosignersAccountVersion[0]) );
			pSubTransaction->Type = mocks::EmbeddedMockTransaction::Entity_Type;
			pSubTransaction->Signer = cosigners[i].publicKey();
			FillWithRandomData(pSubTransaction->Recipient);
			++pSubTransaction;
		}
		return pTransaction;
	}

	/// Creates an aggregate transaction with \a numCosignatures cosignatures, \a numTransactions transactions and populates \a cosigners with the KeyPairs of \a cosignersAccountVersion version, used as transaction signers.
	template<typename TDescriptor>
	model::UniqueEntityPtr<model::AggregateTransaction<TDescriptor>> CreateRandomAggregateTransactionWithCosignatures(uint32_t numCosignatures, uint32_t numTransactions, std::vector<crypto::KeyPair>& cosigners, uint32_t cosignersAccountVersion, bool signerIsCosigner) {
		std::vector<uint32_t> cosignersAccountVersionVector(numCosignatures, cosignersAccountVersion);
		return CreateRandomAggregateTransactionWithCosignatures<TDescriptor>(numCosignatures, numTransactions, cosigners, cosignersAccountVersionVector, signerIsCosigner);
	}


	/// Generates a random cosignature for parent hash (\a aggregateHash).
	template<typename TCosignature>
	TCosignature GenerateValidCosignature(const crypto::KeyPair& keyPair, const Hash256& aggregateHash);

	template<>
	model::Cosignature<SignatureLayout::Raw> GenerateValidCosignature(const crypto::KeyPair& keyPair, const Hash256& aggregateHash);
	template<>
	model::Cosignature<SignatureLayout::Extended> GenerateValidCosignature(const crypto::KeyPair& keyPair, const Hash256& aggregateHash);
	template<>
	model::DetachedCosignature GenerateValidCosignature(const crypto::KeyPair& keyPair, const Hash256& aggregateHash);

	/// Generates a random cosignature for parent hash (\a aggregateHash).
	template<typename TCosignatureType>
	TCosignatureType GenerateValidCosignature(const Hash256& aggregateHash, uint32_t accountVersion) {
		auto keyPair = GenerateKeyPair(accountVersion);
		return GenerateValidCosignature<TCosignatureType>(keyPair, aggregateHash);
	}

	/// Fix cosignatures of \a aggregateTransaction having \a aggregateHash.
	template<typename TDescriptor>
	void FixCosignatures(const std::vector<crypto::KeyPair>& cosigners, const Hash256& aggregateHash, model::AggregateTransaction<TDescriptor>& aggregateTransaction, bool signerIsCosigner) {
		// there must be at least as many transactions in the aggregate transaction as cosigners. If the signer is a cosigner there can be one less.
		auto* pCosignature = aggregateTransaction.CosignaturesPtr();
		for (auto i = signerIsCosigner ? 1u : 0u; i < aggregateTransaction.CosignaturesCount()+ signerIsCosigner ? 1u : 0u; ++i)
		{
			*pCosignature++ = GenerateValidCosignature<typename TDescriptor::CosignatureType>(cosigners[i], aggregateHash);
		}
	}

	/// A map of cosignature components.
	using CosignaturesMap = std::unordered_map<Key, RawSignature, utils::ArrayHasher<Key>>;

	/// Wrapper around an aggregate transaction and its component information.
	template<typename TDescriptor>
	struct AggregateTransactionWrapper {
		/// Aggregate transaction.
		model::UniqueEntityPtr<model::AggregateTransaction<TDescriptor>> pTransaction;

		/// Sub transactions composing the aggregate transaction.
		std::vector<const mocks::EmbeddedMockTransaction*> SubTransactions;
	};

	/// Creates an aggregate transaction composed of \a numTransactions sub transactions.
	template<typename TDescriptor>
	AggregateTransactionWrapper<TDescriptor> CreateAggregateTransaction(uint8_t numTransactions) {
		using TransactionType = model::AggregateTransaction<TDescriptor>;
		uint32_t entitySize = sizeof(TransactionType) + numTransactions * sizeof(mocks::EmbeddedMockTransaction);
		AggregateTransactionWrapper<TDescriptor> wrapper;
		auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);

		pTransaction->Size = entitySize;
		if constexpr(std::is_same_v<TDescriptor, model::AggregateTransactionRawDescriptor>)
		{
			pTransaction->Type = model::Entity_Type_Aggregate_Bonded_V1;
			pTransaction->Version = MakeVersion(model::NetworkIdentifier::Mijin_Test, 3);
		}
		else{
			pTransaction->Type = model::Entity_Type_Aggregate_Bonded_V2;
			pTransaction->Version = MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);
		}
		pTransaction->PayloadSize = numTransactions * sizeof(mocks::EmbeddedMockTransaction);
		FillWithRandomData(pTransaction->Signer);
		FillWithRandomData(pTransaction->Signature);

		auto* pSubTransaction = static_cast<mocks::EmbeddedMockTransaction*>(pTransaction->TransactionsPtr());
		for (auto i = 0u; i < numTransactions; ++i) {
			pSubTransaction->Size = sizeof(mocks::EmbeddedMockTransaction);
			pSubTransaction->Data.Size = 0;
			pSubTransaction->Type = mocks::EmbeddedMockTransaction::Entity_Type;
			FillWithRandomData(pSubTransaction->Signer);
			FillWithRandomData(pSubTransaction->Recipient);

			wrapper.SubTransactions.push_back(pSubTransaction);
			++pSubTransaction;
		}

		wrapper.pTransaction = std::move(pTransaction);
		return wrapper;
	}

	/// Creates a new transaction based on \a aggregateTransaction that excludes all cosignatures.
	template<typename TDescriptor>
	model::UniqueEntityPtr<model::Transaction> StripCosignatures(const model::AggregateTransaction<TDescriptor>& aggregateTransaction) {
		auto pTransactionWithoutCosignatures = CopyEntity(aggregateTransaction);
		uint32_t cosignaturesSize = static_cast<uint32_t>(aggregateTransaction.CosignaturesCount()) * sizeof(typename TDescriptor::CosignatureType);
		pTransactionWithoutCosignatures->Size -= cosignaturesSize;
		return std::move(pTransactionWithoutCosignatures);
	}

	/// Extracts component parts of \a cosignatures into a map.
	template<typename TCosignatureType>
	CosignaturesMap ToMap(const std::vector<TCosignatureType>& cosignatures) {
		CosignaturesMap cosignaturesMap;
		for (const auto& cosignature : cosignatures)
			cosignaturesMap.emplace(cosignature.Signer, cosignature.GetRawSignature());

		return cosignaturesMap;
	}
	template<>
	CosignaturesMap ToMap<model::CosignatureInfo>(const std::vector<model::CosignatureInfo>& cosignatures);


	/// Asserts that \a stitchedTransaction is equal to \a originalTransaction with \a expectedCosignatures.
	/// If nonzero, \a numCosignaturesToIgnore will allow that many cosignatures to be unmatched.
	template<typename TDescriptor>
	void AssertStitchedTransaction(
			const model::Transaction& stitchedTransaction,
			const model::AggregateTransaction<TDescriptor>& originalTransaction,
			const std::vector<model::CosignatureInfo>& expectedCosignatures,
			size_t numCosignaturesToIgnore) {
		// Assert: check basic properties
		auto expectedSize =
				sizeof(model::AggregateTransaction<TDescriptor>)
				+ originalTransaction.PayloadSize
				+ static_cast<uint32_t>((expectedCosignatures.size() + numCosignaturesToIgnore) * sizeof(typename TDescriptor::CosignatureType));
		ASSERT_EQ(expectedSize, stitchedTransaction.Size);
		if constexpr(std::is_same_v<TDescriptor, model::AggregateTransactionRawDescriptor>)
		{
			ASSERT_EQ(model::Entity_Type_Aggregate_Bonded_V1, stitchedTransaction.Type);
		}
		else ASSERT_EQ(model::Entity_Type_Aggregate_Bonded_V2, stitchedTransaction.Type);


		const auto& aggregateStitchedTransaction = static_cast<const model::AggregateTransaction<TDescriptor>&>(stitchedTransaction);

		// - check tx data
		EXPECT_EQ(*StripCosignatures(originalTransaction), *StripCosignatures(aggregateStitchedTransaction));

		// - check cosignatures
		auto expectedCosignaturesMap = ToMap(expectedCosignatures);
		auto numUnknownCosignatures = 0u;
		const auto* pCosignature = aggregateStitchedTransaction.CosignaturesPtr();
		for (auto i = 0u; i < aggregateStitchedTransaction.CosignaturesCount(); ++i) {
			auto message = "cosigner " + ToString(pCosignature->Signer);

			auto iter = expectedCosignaturesMap.find(pCosignature->Signer);
			if (expectedCosignaturesMap.cend() != iter) {
				EXPECT_EQ(iter->first, pCosignature->Signer) << message;
				EXPECT_EQ(iter->second, pCosignature->GetRawSignature()) << message;
				expectedCosignaturesMap.erase(iter);
			} else {
				++numUnknownCosignatures;
			}

			++pCosignature;
		}

		EXPECT_TRUE(expectedCosignaturesMap.empty());
		EXPECT_EQ(numCosignaturesToIgnore, numUnknownCosignatures);
	}
}}
