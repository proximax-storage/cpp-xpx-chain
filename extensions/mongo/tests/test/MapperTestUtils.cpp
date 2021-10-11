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

#include "MapperTestUtils.h"
#include "mongo/src/MongoTransactionMetadata.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/model/Block.h"
#include "catapult/model/ContainerTypes.h"
#include "catapult/model/Cosignature.h"
#include "catapult/model/Receipt.h"
#include "catapult/state/AccountState.h"
#include "tests/test/core/mocks/MockTransaction.h"
#include "tests/TestHarness.h"
#include <bsoncxx/types.hpp>

namespace catapult { namespace test {

	namespace {
		UnresolvedAddress ToUnresolvedAddress(const uint8_t* pByteArray) {
			UnresolvedAddress address;
			std::memcpy(address.data(), pByteArray, Address_Decoded_Size);
			return address;
		}

		template<typename TEntity>
		void AssertEqualEntityData(const TEntity& entity, const bsoncxx::document::view& dbEntity) {
			EXPECT_EQ(entity.Signer, GetKeyValue(dbEntity, "signer"));

			EXPECT_EQ(entity.Version, GetInt32(dbEntity, "version"));
			EXPECT_EQ(utils::to_underlying_type(entity.Type), GetInt32(dbEntity, "type"));
		}

		void AssertEqualHashArray(const std::vector<Hash256>& hashes, const bsoncxx::document::view& dbHashes) {
			ASSERT_EQ(hashes.size(), GetFieldCount(dbHashes));

			auto i = 0u;
			for (const auto& dbHash : dbHashes) {
				Hash256 hash;
				mongo::mappers::DbBinaryToModelArray(hash, dbHash.get_binary());
				EXPECT_EQ(hashes[i], hash);
				++i;
			}
		}
	}

	void AssertEqualEmbeddedTransactionData(const model::EmbeddedTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
		AssertEqualEntityData(transaction, dbTransaction);
	}

	void AssertEqualVerifiableEntityData(const model::VerifiableEntity& entity, const bsoncxx::document::view& dbEntity) {
		EXPECT_EQ(entity.Signature, GetSignatureValue(dbEntity, "signature"));
		AssertEqualEntityData(entity, dbEntity);
	}

	void AssertEqualTransactionData(const model::Transaction& transaction, const bsoncxx::document::view& dbTransaction) {
		AssertEqualVerifiableEntityData(transaction, dbTransaction);
		EXPECT_EQ(transaction.MaxFee, Amount(GetUint64(dbTransaction, "maxFee")));
		EXPECT_EQ(transaction.Deadline, Timestamp(GetUint64(dbTransaction, "deadline")));
	}

	void AssertEqualTransactionMetadata(
			const mongo::MongoTransactionMetadata& metadata,
			const bsoncxx::document::view& dbTransactionMetadata) {
		EXPECT_EQ(metadata.EntityHash, GetHashValue(dbTransactionMetadata, "hash"));
		EXPECT_EQ(metadata.MerkleComponentHash, GetHashValue(dbTransactionMetadata, "merkleComponentHash"));
		auto dbAddresses = dbTransactionMetadata["addresses"].get_array().value;
		model::UnresolvedAddressSet addresses;
		for (const auto& dbAddress : dbAddresses) {
			ASSERT_EQ(Address_Decoded_Size, dbAddress.get_binary().size);
			addresses.insert(ToUnresolvedAddress(dbAddress.get_binary().bytes));
		}

		EXPECT_EQ(metadata.Addresses.size(), addresses.size());
		EXPECT_EQ(metadata.Addresses, addresses);
		EXPECT_EQ(metadata.Height, Height(GetUint64(dbTransactionMetadata, "height")));
		EXPECT_EQ(metadata.Index, GetUint32(dbTransactionMetadata, "index"));
	}

	void AssertEqualBlockData(const model::Block& block, const bsoncxx::document::view& dbBlock) {
		// - 4 fields from VerifiableEntity, 9 fields from Block
		EXPECT_EQ(19u, GetFieldCount(dbBlock));
		AssertEqualVerifiableEntityData(block, dbBlock);

		EXPECT_EQ(block.Height, Height(GetUint64(dbBlock, "height")));
		EXPECT_EQ(block.Timestamp, Timestamp(GetUint64(dbBlock, "timestamp")));
		EXPECT_EQ(block.Difficulty, Difficulty(GetUint64(dbBlock, "difficulty")));
		EXPECT_EQ(block.FeeMultiplier, BlockFeeMultiplier(GetUint32(dbBlock, "feeMultiplier")));
		EXPECT_EQ(block.PreviousBlockHash, GetHashValue(dbBlock, "previousBlockHash"));
		EXPECT_EQ(block.BlockTransactionsHash, GetHashValue(dbBlock, "blockTransactionsHash"));
		EXPECT_EQ(block.BlockReceiptsHash, GetHashValue(dbBlock, "blockReceiptsHash"));
		EXPECT_EQ(block.getGenerationHashProof().Gamma, GetBinaryArray<crypto::ProofGamma::Size>(dbBlock, "proofGamma"));
		EXPECT_EQ(
				block.getGenerationHashProof().VerificationHash,
				GetBinaryArray<crypto::ProofVerificationHash::Size>(dbBlock, "proofVerificationHash"));
		EXPECT_EQ(block.getGenerationHashProof().Scalar, GetBinaryArray<crypto::ProofScalar::Size>(dbBlock, "proofScalar"));
		EXPECT_EQ(block.StateHash, GetHashValue(dbBlock, "stateHash"));
		EXPECT_EQ(block.Beneficiary, GetKeyValue(dbBlock, "beneficiary"));
		EXPECT_EQ(block.FeeInterest, GetUint32(dbBlock, "feeInterest"));
		EXPECT_EQ(block.FeeInterestDenominator, GetUint32(dbBlock, "feeInterestDenominator"));
		EXPECT_EQ(block.transactionPayloadSize(), GetUint32(dbBlock, "transactionPayloadSize"));
	}

	void AssertEqualBlockMetadata(
			const model::BlockElement& blockElement,
			Amount totalFee,
			int32_t numTransactions,
			int32_t numStatements,
			const std::vector<Hash256>& transactionMerkleTree,
			const std::vector<Hash256>& statementMerkleTree,
			const bsoncxx::document::view& dbBlockMetadata) {
		auto expectedFieldCount = statementMerkleTree.empty() ? 6u : 8u;
		EXPECT_EQ(expectedFieldCount, GetFieldCount(dbBlockMetadata));
		EXPECT_EQ(blockElement.EntityHash, GetHashValue(dbBlockMetadata, "hash"));
		EXPECT_EQ(blockElement.GenerationHash, GetGenerationHashValue(dbBlockMetadata, "generationHash"));
		EXPECT_EQ(totalFee, Amount(GetUint64(dbBlockMetadata, "totalFee")));
		EXPECT_EQ(numTransactions, GetInt32(dbBlockMetadata, "numTransactions"));

		AssertEqualHashArray(blockElement.SubCacheMerkleRoots, dbBlockMetadata["subCacheMerkleRoots"].get_array().value);
		AssertEqualHashArray(transactionMerkleTree, dbBlockMetadata["transactionMerkleTree"].get_array().value);
		if (!statementMerkleTree.empty()) {
			EXPECT_EQ(numStatements, GetInt32(dbBlockMetadata, "numStatements"));
			AssertEqualHashArray(statementMerkleTree, dbBlockMetadata["statementMerkleTree"].get_array().value);
		}
	}

	void AssertPublicKeySubDocument(
			const bsoncxx::document::view& dbAccountPublicKeys,
			const std::string& name,
			const state::AccountPublicKeys::PublicKeyAccessor<Key>& expectedPublicKeyAccessor) {
		auto dbPublicKeyDocument = dbAccountPublicKeys[name].get_document();
		EXPECT_EQ(1u, GetFieldCount(dbPublicKeyDocument.view()));
		EXPECT_EQ(expectedPublicKeyAccessor.get(), GetKeyValue(dbPublicKeyDocument.view(), "publicKey"));
	}

	void AssertEqualAccountState(const state::AccountState& accountState, const bsoncxx::document::view& dbAccount) {
		EXPECT_EQ(accountState.Address, GetAddressValue(dbAccount, "address"));
		EXPECT_EQ(accountState.AddressHeight, Height(GetUint64(dbAccount, "addressHeight")));
		EXPECT_EQ(accountState.PublicKey, GetKeyValue(dbAccount, "publicKey"));
		EXPECT_EQ(accountState.PublicKeyHeight, Height(GetUint64(dbAccount, "publicKeyHeight")));
		EXPECT_EQ(accountState.GetVersion(), GetUint32(dbAccount, "version"));
		EXPECT_EQ(accountState.AccountType, static_cast<state::AccountType>(GetInt32(dbAccount, "accountType")));

		auto supplementalKeysDocument = dbAccount["supplementalPublicKeys"].get_document();

		AssertPublicKeySubDocument(supplementalKeysDocument, "linked", accountState.SupplementalPublicKeys.linked());
		if(accountState.GetVersion() > 1)
		{
			AssertPublicKeySubDocument(supplementalKeysDocument, "node", accountState.SupplementalPublicKeys.node());
			AssertPublicKeySubDocument(supplementalKeysDocument, "vrf", accountState.SupplementalPublicKeys.vrf());
		}
		auto dbMosaics = dbAccount["mosaics"].get_array().value;
		size_t numMosaics = 0;
		for (const auto& mosaicElement : dbMosaics) {
			auto mosaicDocument = mosaicElement.get_document();
			auto id = MosaicId(GetUint64(mosaicDocument.view(), "id"));
			EXPECT_EQ(accountState.Balances.get(id), Amount(GetUint64(mosaicDocument.view(), "amount")));
			++numMosaics;
		}

		EXPECT_EQ(accountState.Balances.size(), numMosaics);

		auto dbSnapshots = dbAccount["snapshots"].get_array().value;
		size_t numSnapshots = 0;
		auto snapshotIterator = accountState.Balances.snapshots().begin();
		for (const auto& snapshotsElement : dbSnapshots) {
			auto snapshotsDocument = snapshotsElement.get_document();
			auto amount = GetUint64(snapshotsDocument.view(), "amount");
			auto height = GetUint64(snapshotsDocument.view(), "height");

			EXPECT_EQ(snapshotIterator->Amount, Amount(amount));
			EXPECT_EQ(snapshotIterator->BalanceHeight, Height(height));
			++numSnapshots;
			++snapshotIterator;
		}

		EXPECT_EQ(accountState.Balances.snapshots().size(), numSnapshots);
	}

	void AssertEqualMockTransactionData(const mocks::MockTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
		AssertEqualTransactionData(transaction, dbTransaction);
		EXPECT_EQ(transaction.Recipient, GetKeyValue(dbTransaction, "recipient"));
		EXPECT_EQ(
				ToHexString(transaction.DataPtr(), transaction.Data.Size),
				ToHexString(GetBinary(dbTransaction, "data"), transaction.Data.Size));
	}

	void AssertEqualCosignatures(const std::vector<model::Cosignature<CoSignatureVersionAlias::Raw>>& expectedCosignatures, const bsoncxx::array::view& dbCosignatures) {
		auto iter = dbCosignatures.cbegin();
		for (const auto& expectedCosignature : expectedCosignatures) {
			auto cosignatureView = iter->get_document().view();
			EXPECT_EQ(expectedCosignature.Signer, GetKeyValue(cosignatureView, "signer"));
			EXPECT_EQ(expectedCosignature.Signature, GetSignatureValue(cosignatureView, "signature"));
			++iter;
		}
	}

	void AssertEqualReceiptData(const model::Receipt& receipt, const bsoncxx::document::view& dbReceipt) {
		EXPECT_EQ(receipt.Version, GetInt32(dbReceipt, "version"));
		EXPECT_EQ(utils::to_underlying_type(receipt.Type), GetInt32(dbReceipt, "type"));
	}
}}
