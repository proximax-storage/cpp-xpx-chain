/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeTransactionsTestUtils.h"
#include "src/model/ExchangeTransaction.h"

namespace catapult { namespace model {

	using TransactionType = ExchangeTransaction;

#define TEST_CLASS ExchangeTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			AssertTransactionHasExpectedProperties<T>(Entity_Type_Exchange);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(Exchange)

	// endregion

	// region data pointers

	namespace {
		using ExchangeTransactionTraits = ExchangeTransactionsTraits<ExchangeTransaction, MatchedOffer>;
	}

	DEFINE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, ExchangeTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		AssertCanCalculateRealSizeWithReasonableValues<TransactionType, MatchedOffer>();
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		AssertCalculateRealSizeDoesNotOverflowWithMaxValues<TransactionType, MatchedOffer>();
	}

	// endregion
}}
