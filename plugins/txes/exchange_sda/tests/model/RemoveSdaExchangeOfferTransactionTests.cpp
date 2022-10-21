/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaExchangeTransactionsTestUtils.h"
#include "src/model/RemoveSdaExchangeOfferTransaction.h"

namespace catapult { namespace model {

	using TransactionType = RemoveSdaExchangeOfferTransaction;

#define TEST_CLASS RemoveSdaExchangeOfferTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			AssertTransactionHasExpectedProperties<T>(Entity_Type_Remove_Sda_Exchange_Offer, 1);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(RemoveSdaExchangeOffer)

	// endregion

	// region data pointers

	namespace {
		using RemoveSdaExchangeOfferTransactionTraits = ExchangeTransactionsTraits<RemoveSdaExchangeOfferTransaction, SdaOfferMosaic>;
	}

	DEFINE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, RemoveSdaExchangeOfferTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		AssertCanCalculateRealSizeWithReasonableValues<TransactionType, SdaOfferMosaic>();
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		AssertCalculateRealSizeDoesNotOverflowWithMaxValues<TransactionType, SdaOfferMosaic>();
	}

	// endregion
}}
