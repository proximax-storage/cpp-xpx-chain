/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/EndDriveVerificationTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionContainerTestUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

#define TEST_CLASS EndDriveVerificationTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize = baseSize; // base

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_End_Drive_Verification, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(EndDriveVerification)

	// endregion

#define DATA_POINTER_TEST(TEST_NAME) \
	template<typename T> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Regular) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<EndDriveVerificationTransaction>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Embedded) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<EmbeddedEndDriveVerificationTransaction>(); } \
	template<typename T> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	namespace {
		static constexpr auto Num_Failures = 3u;
		static constexpr auto Block_Hash_Count = 5u;

		template<typename T>
		VerificationFailure& GetSecondFailure(T& transaction) {
			uint8_t* pData = reinterpret_cast<uint8_t*>(&transaction + 1);
			return *reinterpret_cast<VerificationFailure*>(pData + reinterpret_cast<VerificationFailure*>(pData)->Size);
		}
	}

	// region verification failures

	DATA_POINTER_TEST(FailuresAreInaccessibleWhenTransactionHasNoFailures) {
		// Arrange:
		auto pTransaction = test::CreateEndDriveVerificationTransaction<T>(0);

		// Act + Assert:
		EXPECT_FALSE(!!pTransaction->TransactionsPtr());
		EXPECT_EQ(0u, test::CountTransactions(pTransaction->Transactions()));
	}

	DATA_POINTER_TEST(FailuresAreAccessibleWhenTransactionHasFailures) {
		// Arrange:
		auto pTransaction = test::CreateEndDriveVerificationTransaction<T>(Num_Failures, Block_Hash_Count);
		const auto* pTransactionEnd = test::AsVoidPointer(pTransaction.get() + 1);

		// Act + Assert:
		EXPECT_EQ(pTransactionEnd, pTransaction->TransactionsPtr());
		EXPECT_EQ(Num_Failures, test::CountTransactions(pTransaction->Transactions()));
	}

	// endregion

	// region CalculateRealSize - no verification failures

	DATA_POINTER_TEST(SizeInvalidWhenReportedSizeIsZero) {
		// Arrange:
		auto pTransaction = test::CreateEndDriveVerificationTransaction<T>(0);
		pTransaction->Size = 0;

		// Act + Assert:
		EXPECT_EQ(std::numeric_limits<uint64_t>::max(), T::CalculateRealSize(*pTransaction));
	}

	DATA_POINTER_TEST(SizeInvalidWhenReportedSizeIsLessThanHeaderSize) {
		// Arrange:
		auto pTransaction = test::CreateEndDriveVerificationTransaction<T>(0);
		--pTransaction->Size;

		// Act + Assert:
		EXPECT_EQ(std::numeric_limits<uint64_t>::max(), T::CalculateRealSize(*pTransaction));
	}

	DATA_POINTER_TEST(SizeValidWhenReportedSizeIsEqualToHeaderSize) {
		// Arrange:
		auto pTransaction = test::CreateEndDriveVerificationTransaction<T>(0);

		// Act + Assert:
		EXPECT_EQ(sizeof(T), T::CalculateRealSize(*pTransaction));
	}

	// endregion

	// region CalculateRealSize - verification failure sizes

	DATA_POINTER_TEST(SizeInvalidWhenAnyFailureHasZeroSize) {
		// Arrange:
		auto pTransaction = test::CreateEndDriveVerificationTransaction<T>(Num_Failures, Block_Hash_Count);
		GetSecondFailure(*pTransaction).Size = 0;

		// Act + Assert:
		EXPECT_EQ(std::numeric_limits<uint64_t>::max(), T::CalculateRealSize(*pTransaction));
	}

	DATA_POINTER_TEST(SizeInvalidWhenAnyFailureHasInvalidSize) {
		// Arrange:
		auto pTransaction = test::CreateEndDriveVerificationTransaction<T>(Num_Failures, Block_Hash_Count);
		GetSecondFailure(*pTransaction).Size = 1;

		// Act + Assert:
		EXPECT_EQ(std::numeric_limits<uint64_t>::max(), T::CalculateRealSize(*pTransaction));
	}

	DATA_POINTER_TEST(SizeInvalidWhenAnyFailureExpandsBeyondBuffer) {
		// Arrange:
		auto pTransaction = test::CreateEndDriveVerificationTransaction<T>(Num_Failures, Block_Hash_Count);
		GetSecondFailure(*pTransaction).Size = pTransaction->Size - pTransaction->TransactionsPtr()->Size + 1;

		// Act + Assert:
		EXPECT_EQ(std::numeric_limits<uint64_t>::max(), T::CalculateRealSize(*pTransaction));
	}

	DATA_POINTER_TEST(SizeValidWhenReportedSizeIsEqualToHeaderSizePlusFailureSizes) {
		// Arrange:
		auto pTransaction = test::CreateEndDriveVerificationTransaction<T>(Num_Failures, Block_Hash_Count);

		// Act + Assert:
		EXPECT_EQ(sizeof(T) + Num_Failures * (sizeof(model::VerificationFailure) + Block_Hash_Count * sizeof(Hash256)), T::CalculateRealSize(*pTransaction));
	}

	// endregion
}}
