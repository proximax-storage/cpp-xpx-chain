/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeTransactionsTestUtils.h"
#include "src/model/ExchangeOfferTransaction.h"

namespace catapult { namespace model {

	using TransactionType = ExchangeOfferTransaction;

#define TEST_CLASS ExchangeOfferTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			AssertTransactionHasExpectedProperties<T>(Entity_Type_Exchange_Offer);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(ExchangeOffer)

	// endregion

	// region data pointers

	namespace {
		using ExchangeOfferTransactionTraits = ExchangeTransactionsTraits<ExchangeOfferTransaction, OfferWithDuration>;
	}

	DEFINE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, ExchangeOfferTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		AssertCanCalculateRealSizeWithReasonableValues<TransactionType, OfferWithDuration>();
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		AssertCalculateRealSizeDoesNotOverflowWithMaxValues<TransactionType, OfferWithDuration>();
	}

	// endregion
}}
