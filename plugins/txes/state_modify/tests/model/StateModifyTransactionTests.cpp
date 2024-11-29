/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/ModifyStateTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "catapult/utils/MemoryUtils.h"

namespace catapult { namespace model {

	using TransactionType = ModifyStateTransaction;

#define TEST_CLASS StateModifyTransactionTests

	// region size + properties
	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedPropertiesHeaderSize =
					sizeof(uint16_t) // cache name size
					+ sizeof(uint8_t) // subn cache id
					+ sizeof(uint32_t) // key size
					+ sizeof(uint32_t); // content size
			auto expectedSize =
					baseSize // base
					+ expectedPropertiesHeaderSize;

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 11u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_ModifyState, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(ModifyState)

	// endregion

	// region data pointers

	namespace {
		struct StateModifyTransactionTraits {
			static auto GenerateEntityWithAttachments(uint8_t cacheNameSize, uint32_t keySize, uint32_t contentSize) {
				uint32_t entitySize = sizeof(TransactionType) + cacheNameSize + keySize + contentSize;
				auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
				pTransaction->Size = entitySize;
				pTransaction->KeySize = keySize;
				pTransaction->CacheNameSize = cacheNameSize;
				pTransaction->ContentSize = contentSize;
				return pTransaction;
			}
			static constexpr size_t GetAttachment1Size(uint8_t cacheNameSize) {
				return cacheNameSize;
			}

			static constexpr size_t GetAttachment2Size(uint32_t keySize) {
				return keySize;
			}

			static constexpr size_t GetAttachment3Size(uint32_t contentSize) {
				return contentSize;
			}
			template<typename TEntity>
			static auto GetAttachmentPointer1(TEntity& entity) {
				return entity.CacheNamePtr();
			}

			template<typename TEntity>
			static auto GetAttachmentPointer2(TEntity& entity) {
				return entity.KeyPtr();
			}

			template<typename TEntity>
			static auto GetAttachmentPointer3(TEntity& entity) {
				return entity.ContentPtr();
			}
		};
	}

	DEFINE_TRIPLE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, StateModifyTransactionTraits)

	// endregion

	// region CalculateRealSize


	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		TransactionType transaction;
		transaction.Size = 0;
		transaction.KeySize = 10;
		transaction.CacheNameSize = 20;
		transaction.ContentSize = 30;

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(TransactionType) + 10 + 20 + 30, realSize);
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
		TransactionType transaction;
		test::SetMaxValue(transaction.Size);
		test::SetMaxValue(transaction.KeySize);
		test::SetMaxValue(transaction.CacheNameSize);
		test::SetMaxValue(transaction.ContentSize);

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		ASSERT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(TransactionType) + 0xFF + 0xFFFFFFFF + 0xFFFFFFFF, realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
	}

	// endregion
}}
