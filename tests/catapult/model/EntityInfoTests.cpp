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

#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/nodeps/Equality.h"

namespace catapult { namespace model {

#define TEST_CLASS EntityInfoTests

	// region general

	TEST(TEST_CLASS, CanCreateDefaultEntityInfo) {
		// Arrange + Act:
		EntityInfo<Block> entityInfo;

		// Assert: notice that hash is not zero-initialized by default constructor
		EXPECT_FALSE(!!entityInfo);
		EXPECT_FALSE(!!entityInfo.pEntity);
	}

	TEST(TEST_CLASS, CanCreateEntityInfo) {
		// Arrange:
		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
		auto hash = test::GenerateRandomByteArray<Hash256>();

		// Act:
		EntityInfo<Block> entityInfo(pBlock, hash);

		// Assert:
		EXPECT_TRUE(!!entityInfo);
		EXPECT_EQ(pBlock.get(), entityInfo.pEntity.get());
		EXPECT_EQ(hash, entityInfo.EntityHash);
	}

	TEST(TEST_CLASS, CanMoveConstructEntityInfo) {
		// Arrange:
		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
		auto hash = test::GenerateRandomByteArray<Hash256>();
		EntityInfo<Block> original(pBlock, hash);

		// Act:
		EntityInfo<Block> entityInfo(std::move(original));

		// Assert:
		EXPECT_TRUE(!!entityInfo);
		EXPECT_EQ(pBlock.get(), entityInfo.pEntity.get());
		EXPECT_EQ(hash, entityInfo.EntityHash);
	}

	// endregion

	// region hasher

	TEST(TEST_CLASS, EntityInfoHasher_SameObjectReturnsSameHash) {
		// Arrange:
		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
		EntityInfo<Block> entityInfo(pBlock, test::GenerateRandomByteArray<Hash256>());
		EntityInfoHasher<Block> hasher;

		// Act:
		auto result1 = hasher(entityInfo);
		auto result2 = hasher(entityInfo);

		// Assert:
		EXPECT_EQ(result1, result2);
	}

	TEST(TEST_CLASS, EntityInfoHasher_EqualObjectsReturnSameHash) {
		// Arrange:
		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
		auto hash = test::GenerateRandomByteArray<Hash256>();
		EntityInfo<Block> entityInfo1(pBlock, hash);
		EntityInfo<Block> entityInfo2(pBlock, hash);
		EntityInfoHasher<Block> hasher;

		// Act:
		auto result1 = hasher(entityInfo1);
		auto result2 = hasher(entityInfo2);

		// Assert:
		EXPECT_EQ(result1, result2);
	}

	TEST(TEST_CLASS, EntityInfoHasher_DifferentObjectsReturnDifferentHashes) {
		// Arrange:
		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
		EntityInfo<Block> entityInfo1(pBlock, test::GenerateRandomByteArray<Hash256>());
		EntityInfo<Block> entityInfo2(pBlock, test::GenerateRandomByteArray<Hash256>());
		EntityInfoHasher<Block> hasher;

		// Act:
		auto result1 = hasher(entityInfo1);
		auto result2 = hasher(entityInfo2);

		// Assert:
		EXPECT_NE(result1, result2);
	}

	// endregion

	// region comparer

	TEST(TEST_CLASS, EntityInfoComparer_ReturnsTrueForObjectsWithEqualHashes) {
		// Arrange:
		auto pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
		auto hash = test::GenerateRandomByteArray<Hash256>();
		std::unordered_map<std::string, EntityInfo<Block>> map;
		map.emplace("default", EntityInfo<Block>(pBlock, hash));
		map.emplace("copy", EntityInfo<Block>(pBlock, hash));
		map.emplace("diff-block", EntityInfo<Block>(utils::UniqueToShared(test::GenerateEmptyRandomBlock()), hash));
		map.emplace("diff-hash", EntityInfo<Block>(pBlock, test::GenerateRandomByteArray<Hash256>()));

		// Assert:
		test::AssertEqualReturnsTrueForEqualObjects<EntityInfo<Block>>(
				"default",
				map,
				{ "default", "copy", "diff-block" },
				EntityInfoComparer<Block>());
	}

	// endregion

	// region transaction

	TEST(TEST_CLASS, CanCreateDefaultTransactionInfo_Detached) {
		// Act:
		DetachedTransactionInfo transactionInfo;

		// Assert: notice that hash(es) are not zero-initialized by default constructor
		EXPECT_FALSE(!!transactionInfo.pEntity);
		EXPECT_FALSE(!!transactionInfo.OptionalExtractedAddresses);
	}

	TEST(TEST_CLASS, CanCreateTransactionInfoWithoutHash_Detached) {
		// Arrange:
		auto pTransaction = utils::UniqueToShared(test::GenerateRandomTransaction());

		// Act:
		DetachedTransactionInfo transactionInfo(pTransaction);

		// Assert:
		EXPECT_EQ(pTransaction.get(), transactionInfo.pEntity.get());
		EXPECT_EQ(Hash256(), transactionInfo.EntityHash);
		EXPECT_FALSE(!!transactionInfo.OptionalExtractedAddresses);
	}

	TEST(TEST_CLASS, CanCreateTransactionInfoWithHash_Detached) {
		// Arrange:
		auto pTransaction = utils::UniqueToShared(test::GenerateRandomTransaction());
		auto entityHash = test::GenerateRandomByteArray<Hash256>();

		// Act:
		DetachedTransactionInfo transactionInfo(pTransaction, entityHash);

		// Assert:
		EXPECT_EQ(pTransaction.get(), transactionInfo.pEntity.get());
		EXPECT_EQ(entityHash, transactionInfo.EntityHash);
		EXPECT_FALSE(!!transactionInfo.OptionalExtractedAddresses);
	}

	TEST(TEST_CLASS, CanCreateDefaultTransactionInfo) {
		// Act:
		TransactionInfo transactionInfo;

		// Assert: notice that hash(es) are not zero-initialized by default constructor
		EXPECT_FALSE(!!transactionInfo.pEntity);
		EXPECT_FALSE(!!transactionInfo.OptionalExtractedAddresses);
	}

	TEST(TEST_CLASS, CanCreateTransactionInfoWithoutHash) {
		// Arrange:
		auto pTransaction = utils::UniqueToShared(test::GenerateRandomTransaction());

		// Act:
		TransactionInfo transactionInfo(pTransaction, Height(123));

		// Assert:
		EXPECT_EQ(pTransaction.get(), transactionInfo.pEntity.get());
		EXPECT_EQ(Hash256(), transactionInfo.EntityHash);
		EXPECT_FALSE(!!transactionInfo.OptionalExtractedAddresses);
		// Assert: check merkle component hash
		EXPECT_EQ(Hash256(), transactionInfo.MerkleComponentHash);
		EXPECT_EQ(Height(123), transactionInfo.AssociatedHeight);
	}

	TEST(TEST_CLASS, CanCreateTransactionInfoWithHash) {
		// Arrange:
		auto pTransaction = utils::UniqueToShared(test::GenerateRandomTransaction());
		auto entityHash = test::GenerateRandomByteArray<Hash256>();

		// Act:
		TransactionInfo transactionInfo(pTransaction, entityHash, Height(123));

		// Assert:
		EXPECT_EQ(pTransaction.get(), transactionInfo.pEntity.get());
		EXPECT_EQ(entityHash, transactionInfo.EntityHash);
		EXPECT_FALSE(!!transactionInfo.OptionalExtractedAddresses);
		// Assert: check merkle component hash
		EXPECT_EQ(Hash256(), transactionInfo.MerkleComponentHash);
		EXPECT_EQ(Height(123), transactionInfo.AssociatedHeight);
	}

	TEST(TEST_CLASS, CanCopyDetachedTransactionInfo) {
		// Arrange:
		auto pTransaction = utils::UniqueToShared(test::GenerateRandomTransaction());
		auto entityHash = test::GenerateRandomByteArray<Hash256>();
		auto pExtractedAddresses = std::make_shared<UnresolvedAddressSet>();

		DetachedTransactionInfo transactionInfo(pTransaction, entityHash);
		transactionInfo.OptionalExtractedAddresses = pExtractedAddresses;

		// Act:
		auto transactionInfoCopy = transactionInfo.copy();

		// Assert:
		// - original info is unmodified
		EXPECT_EQ(pTransaction.get(), transactionInfo.pEntity.get());
		EXPECT_EQ(entityHash, transactionInfo.EntityHash);
		EXPECT_EQ(pExtractedAddresses.get(), transactionInfo.OptionalExtractedAddresses.get());

		// - copied info has correct values
		EXPECT_EQ(pTransaction.get(), transactionInfoCopy.pEntity.get());
		EXPECT_EQ(entityHash, transactionInfoCopy.EntityHash);
		EXPECT_EQ(pExtractedAddresses.get(), transactionInfoCopy.OptionalExtractedAddresses.get());
	}

	TEST(TEST_CLASS, CanCopyTransactionInfo) {
		// Arrange:
		auto pTransaction = utils::UniqueToShared(test::GenerateRandomTransaction());
		auto entityHash = test::GenerateRandomByteArray<Hash256>();
		auto pExtractedAddresses = std::make_shared<UnresolvedAddressSet>();
		auto merkleComponentHash = test::GenerateRandomByteArray<Hash256>();
		auto height = Height(123);

		TransactionInfo transactionInfo(pTransaction, entityHash, height);
		transactionInfo.OptionalExtractedAddresses = pExtractedAddresses;
		transactionInfo.MerkleComponentHash = merkleComponentHash;

		// Act:
		auto transactionInfoCopy = transactionInfo.copy();

		// Assert:
		// - original info is unmodified
		EXPECT_EQ(pTransaction.get(), transactionInfo.pEntity.get());
		EXPECT_EQ(entityHash, transactionInfo.EntityHash);
		EXPECT_EQ(pExtractedAddresses.get(), transactionInfo.OptionalExtractedAddresses.get());
		EXPECT_EQ(merkleComponentHash, transactionInfo.MerkleComponentHash);
		EXPECT_EQ(height, transactionInfo.AssociatedHeight);

		// - copied info has correct values
		EXPECT_EQ(pTransaction.get(), transactionInfoCopy.pEntity.get());
		EXPECT_EQ(entityHash, transactionInfoCopy.EntityHash);
		EXPECT_EQ(pExtractedAddresses.get(), transactionInfoCopy.OptionalExtractedAddresses.get());
		EXPECT_EQ(merkleComponentHash, transactionInfoCopy.MerkleComponentHash);
		EXPECT_EQ(height, transactionInfoCopy.AssociatedHeight);
	}

	// endregion
}}
