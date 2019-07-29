/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/CatapultConfigTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

	using TransactionType = CatapultConfigTransaction;

#define TEST_CLASS CatapultConfigTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize = baseSize // base
					+ sizeof(int64_t) // duration delta
					+ sizeof(uint16_t) // blockchain config size
					+ sizeof(uint16_t); // supported entity versions size

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 12u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_Catapult_Config, static_cast<EntityType>(T::Entity_Type));
			EXPECT_EQ(1u, static_cast<VersionType>(T::Current_Version));
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(CatapultConfig)

	// endregion

	// region data pointers

	namespace {
		struct CatapultConfigTransactionTraits {
			static auto GenerateEntityWithAttachments(uint16_t blockChainConfigSize, uint16_t supportedEntityVersionsSize) {
				uint32_t entitySize = sizeof(TransactionType) + blockChainConfigSize + supportedEntityVersionsSize;
				auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
				pTransaction->Size = entitySize;
				pTransaction->BlockChainConfigSize = blockChainConfigSize;
				pTransaction->SupportedEntityVersionsSize = supportedEntityVersionsSize;
				return pTransaction;
			}

			static constexpr size_t GetAttachment1Size(uint16_t blockChainConfigSize) {
				return blockChainConfigSize;
			}

			template<typename TEntity>
			static auto GetAttachmentPointer1(TEntity& entity) {
				return entity.BlockChainConfigPtr();
			}

			template<typename TEntity>
			static auto GetAttachmentPointer2(TEntity& entity) {
				return entity.SupportedEntityVersionsPtr();
			}
		};
	}

	DEFINE_DUAL_ATTACHMENT_POINTER_TESTS(TEST_CLASS, CatapultConfigTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		TransactionType transaction;
		transaction.Size = 0;
		transaction.BlockChainConfigSize = 2;
		transaction.SupportedEntityVersionsSize = 4;

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(TransactionType) + 2 + 4, realSize);
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
		TransactionType transaction;
		test::SetMaxValue(transaction.Size);
		test::SetMaxValue(transaction.BlockChainConfigSize);
		test::SetMaxValue(transaction.SupportedEntityVersionsSize);

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(TransactionType) + 2 * 0xFFFF, realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
	}

	// endregion
}}
