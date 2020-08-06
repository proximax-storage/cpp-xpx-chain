/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeTransactionsTestUtils.h"
#include "src/model/RemoveExchangeOfferTransaction.h"

namespace catapult { namespace model {

	using TransactionType = RemoveExchangeOfferTransaction;

#define TEST_CLASS RemoveExchangeOfferTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			AssertTransactionHasExpectedProperties<T>(Entity_Type_Remove_Exchange_Offer, 2);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(RemoveExchangeOffer)

	// endregion

	// region data pointers

	namespace {
		using RemoveExchangeOfferTransactionTraits = ExchangeTransactionsTraits<RemoveExchangeOfferTransaction, OfferMosaic>;
	}

	DEFINE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, RemoveExchangeOfferTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		AssertCanCalculateRealSizeWithReasonableValues<TransactionType, OfferMosaic>();
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		AssertCalculateRealSizeDoesNotOverflowWithMaxValues<TransactionType, OfferMosaic>();
	}

	// endregion
}}
