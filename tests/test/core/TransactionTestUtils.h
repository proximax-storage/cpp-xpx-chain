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
#include "AddressTestUtils.h"
#include "catapult/model/Cosignature.h"
#include "catapult/model/RangeTypes.h"
#include "tests/TestHarness.h"
#include <memory>
#include <vector>

namespace catapult { namespace test {

	using ConstTransactions = std::vector<std::shared_ptr<const model::Transaction>>;
	using MutableTransactions = std::vector<std::shared_ptr<model::Transaction>>;

	/// Hash string of the deterministic transaction.
#ifdef SIGNATURE_SCHEME_NIS1
	constexpr auto Deterministic_Transaction_Hash_String = "6EA6D56912DF29CDE28A182F091DC952CE4B74BFA1A4E11D1F606C79F4A57549";
#else
	constexpr auto Deterministic_Transaction_Hash_String = "928C1370941AAACE99C91D31D6C6B4FA511F83387C4DA99536F8F0B62545D755";
#endif

	/// Gets default generation hash used in tests.
	GenerationHash GetDefaultGenerationHash();

	/// Generates a transaction with random data.
	std::unique_ptr<model::Transaction> GenerateRandomTransaction();

	/// Generates a transaction for a network with specified generation hash (\a generationHash).
	std::unique_ptr<model::Transaction> GenerateRandomTransaction(const GenerationHash& generationHash);

	/// Generates a transaction with random data around \a signer.
	std::unique_ptr<model::Transaction> GenerateRandomTransaction(const Key& signer);

	/// Generates \a count transactions with random data.
	MutableTransactions GenerateRandomTransactions(size_t count);

	/// Returns const transactions container composed of all the mutable transactions in \a transactions.
	ConstTransactions MakeConst(const MutableTransactions& transactions);

	/// Generates a random transaction with size \a entitySize.
	std::unique_ptr<model::Transaction> GenerateRandomTransactionWithSize(size_t entitySize);

	/// Generates a transaction with \a deadline.
	std::unique_ptr<model::Transaction> GenerateTransactionWithDeadline(Timestamp deadline);

	/// Generates a predefined transaction, i.e. this function will always return the same transaction.
	std::unique_ptr<model::Transaction> GenerateDeterministicTransaction();

	/// Policy for creating a transaction.
	struct TransactionPolicy {
		static auto Create() {
			return GenerateRandomTransaction();
		}
	};

	/// Creates a transaction entity range composed of \a numTransactions transactions.
	model::TransactionRange CreateTransactionEntityRange(size_t numTransactions);

	/// Copies \a transactions into an entity range.
	model::TransactionRange CreateEntityRange(const std::vector<const model::Transaction*>& transactions);

	/// Creates a random (detached) cosignature.
	model::DetachedCosignature CreateRandomCosignature();

/// Adds basic transaction size and property tests for \a NAME transaction.
#define ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(NAME) \
	TEST(NAME##TransactionTests, EntityHasExpectedSize) { \
		AssertEntityHasExpectedSize<NAME##Transaction>(sizeof(Transaction)); \
	} \
	TEST(NAME##TransactionTests, TransactionHasExpectedProperties) { \
		AssertTransactionHasExpectedProperties<NAME##Transaction>(); \
	} \
	TEST(NAME##TransactionTests, EmbeddedTransactionHasExpectedSize) { \
		AssertEntityHasExpectedSize<Embedded##NAME##Transaction>(sizeof(EmbeddedTransaction)); \
	} \
	TEST(NAME##TransactionTests, EmbeddedTransactionHasExpectedProperties) { \
		AssertTransactionHasExpectedProperties<Embedded##NAME##Transaction>(); \
	}
}}
