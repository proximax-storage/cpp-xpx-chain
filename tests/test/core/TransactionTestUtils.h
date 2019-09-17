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
#include "catapult/model/EntityPtr.h"
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
	constexpr auto Deterministic_Transaction_Hash_String = "85168DC0940A111229F05E4DFE3C8F8F1193E811D6AC7D14B43A7BACB3A93054";
#endif

	/// Gets default generation hash used in tests.
	GenerationHash GetDefaultGenerationHash();

	/// Generates a transaction with random data.
	model::UniqueEntityPtr<model::Transaction> GenerateRandomTransaction();

	/// Generates a transaction for a network with specified generation hash (\a generationHash).
	model::UniqueEntityPtr<model::Transaction> GenerateRandomTransaction(const GenerationHash& generationHash);

	/// Generates a transaction with random data around \a signer.
	model::UniqueEntityPtr<model::Transaction> GenerateRandomTransaction(const Key& signer);

	/// Generates \a count transactions with random data.
	MutableTransactions GenerateRandomTransactions(size_t count);

	/// Returns const transactions container composed of all the mutable transactions in \a transactions.
	ConstTransactions MakeConst(const MutableTransactions& transactions);

	/// Generates a random transaction with size \a entitySize.
	model::UniqueEntityPtr<model::Transaction> GenerateRandomTransactionWithSize(size_t entitySize);

	/// Generates a transaction with \a deadline.
	model::UniqueEntityPtr<model::Transaction> GenerateTransactionWithDeadline(Timestamp deadline);

	/// Generates a predefined transaction, i.e. this function will always return the same transaction.
	model::UniqueEntityPtr<model::Transaction> GenerateDeterministicTransaction();

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
