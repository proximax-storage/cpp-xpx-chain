/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/SdaOffer.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

	template<typename TTransaction>
	void AssertEntityHasExpectedSize(size_t baseSize) {
		// Arrange:
		auto expectedSize = baseSize // base
			+ 1; // offer count

		// Assert:
		EXPECT_EQ(expectedSize, sizeof(TTransaction));
		EXPECT_EQ(baseSize + 1u, sizeof(TTransaction));
	}

	template<typename TTransaction>
	void AssertTransactionHasExpectedProperties(EntityType entityType, VersionType version = 1) {
		// Assert:
		EXPECT_EQ(entityType, TTransaction::Entity_Type);
		EXPECT_EQ(version, TTransaction::Current_Version);
	}

	template<typename TTransaction, typename TOffer>
	struct ExchangeTransactionsTraits {
		static auto GenerateEntityWithAttachments(uint8_t offerCount) {
			uint32_t entitySize = sizeof(TTransaction) + offerCount * sizeof(TOffer);
			auto pTransaction = utils::MakeUniqueWithSize<TTransaction>(entitySize);
			pTransaction->Size = entitySize;
			pTransaction->OfferCount = offerCount;
			return pTransaction;
		}

		template<typename TEntity>
		static auto GetAttachmentPointer(TEntity& entity) {
			return entity.OffersPtr();
		}
	};

	template<typename TTransaction, typename TOffer>
	void AssertCanCalculateRealSizeWithReasonableValues() {
		// Arrange:
		TTransaction transaction;
		transaction.Size = 0;
		transaction.OfferCount = 4;

		// Act:
		auto realSize = TTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(TTransaction) + 4 * sizeof(TOffer), realSize);
	}

	template<typename TTransaction, typename TOffer>
	void AssertCalculateRealSizeDoesNotOverflowWithMaxValues() {
		// Arrange:
		TTransaction transaction;
		test::SetMaxValue(transaction.Size);
		test::SetMaxValue(transaction.OfferCount);

		// Act:
		auto realSize = TTransaction::CalculateRealSize(transaction);

		// Assert:
		ASSERT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(TTransaction) + 0xFF * sizeof(TOffer), realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
	}
}}
