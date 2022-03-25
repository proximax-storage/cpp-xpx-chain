/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaExchangeTransactionsTestUtils.h"
#include "src/model/PlaceSdaExchangeOfferTransaction.h"

namespace catapult { namespace model {

	using TransactionType = PlaceSdaExchangeOfferTransaction;

#define TEST_CLASS PlaceSdaExchangeOfferTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			AssertTransactionHasExpectedProperties<T>(Entity_Type_Place_Sda_Exchange_Offer, 1);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(PlaceSdaExchangeOffer)

	// endregion

	// region data pointers

	namespace {
		using PlaceSdaExchangeOfferTransactionTraits = ExchangeTransactionsTraits<PlaceSdaExchangeOfferTransaction, SdaOfferWithOwnerAndDuration>;
	}

	DEFINE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, PlaceSdaExchangeOfferTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		AssertCanCalculateRealSizeWithReasonableValues<TransactionType, SdaOfferWithOwnerAndDuration>();
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		AssertCalculateRealSizeDoesNotOverflowWithMaxValues<TransactionType, SdaOfferWithOwnerAndDuration>();
	}

	// endregion
}}
