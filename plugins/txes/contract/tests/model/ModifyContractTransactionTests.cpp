/**
*** Copyright (c) 2018-present,
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

#include "src/model/ModifyContractTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

	using TransactionType = ModifyContractTransaction;

#define TEST_CLASS ModifyContractTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize = baseSize // base
					+ sizeof(int64_t) // duration delta
					+ sizeof(Hash256) // hash
					+ sizeof(uint8_t) // customer modification count
					+ sizeof(uint8_t) // executor modification count
					+ sizeof(uint8_t); // verifier modification count

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 43u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_Modify_Contract, static_cast<EntityType>(T::Entity_Type));
			EXPECT_EQ(3u, static_cast<uint8_t>(T::Current_Version));
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(ModifyContract)

	// endregion

	// region data pointers

	namespace {
		struct ModifyContractTransactionTraits {
			static auto GenerateEntityWithAttachments(
					uint8_t customerModificationCount,
					uint8_t executorModificationCount,
					uint8_t verifierModificationCount) {
				uint32_t entitySize = sizeof(TransactionType)
					+ customerModificationCount * sizeof(CosignatoryModification)
					+ executorModificationCount * sizeof(CosignatoryModification)
					+ verifierModificationCount * sizeof(CosignatoryModification);
				auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
				pTransaction->Size = entitySize;
				pTransaction->CustomerModificationCount = customerModificationCount;
				pTransaction->ExecutorModificationCount = executorModificationCount;
				pTransaction->VerifierModificationCount = verifierModificationCount;
				return pTransaction;
			}

			static constexpr size_t GetAttachment1Size(uint8_t numModifications) {
				return numModifications * sizeof(CosignatoryModification);
			}

			static constexpr size_t GetAttachment2Size(uint8_t numModifications) {
				return GetAttachment1Size(numModifications);
			}

			template<typename TEntity>
			static auto GetAttachmentPointer1(TEntity& entity) {
				return entity.CustomerModificationsPtr();
			}

			template<typename TEntity>
			static auto GetAttachmentPointer2(TEntity& entity) {
				return entity.ExecutorModificationsPtr();
			}

			template<typename TEntity>
			static auto GetAttachmentPointer3(TEntity& entity) {
				return entity.VerifierModificationsPtr();
			}
		};
	}

	DEFINE_TRIPLE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, ModifyContractTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		TransactionType transaction;
		transaction.Size = 0;
		transaction.CustomerModificationCount = 2;
		transaction.ExecutorModificationCount = 4;
		transaction.VerifierModificationCount = 5;

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(33u, sizeof(CosignatoryModification));
		EXPECT_EQ(sizeof(TransactionType) + 11 * sizeof(CosignatoryModification), realSize);
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
		TransactionType transaction;
		test::SetMaxValue(transaction.Size);
		test::SetMaxValue(transaction.CustomerModificationCount);
		test::SetMaxValue(transaction.ExecutorModificationCount);
		test::SetMaxValue(transaction.VerifierModificationCount);

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(TransactionType) + 3 * 0xFF * sizeof(CosignatoryModification), realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
	}

	// endregion
}}
