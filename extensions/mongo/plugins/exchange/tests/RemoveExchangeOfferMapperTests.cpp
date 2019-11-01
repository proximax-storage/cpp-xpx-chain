/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/RemoveExchangeOfferMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "plugins/txes/exchange/src/model/RemoveExchangeOfferTransaction.h"
#include "plugins/txes/exchange/tests/test/ExchangeTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS RemoveExchangeOfferMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(RemoveExchangeOffer,)

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			auto iter = dbTransaction["offers"].get_array().value.cbegin();
			const model::OfferMosaic* pOffer = transaction.OffersPtr();
			for (uint8_t i = 0; i < transaction.OfferCount; ++i, ++pOffer, ++iter) {
				const auto& doc = iter->get_document().view();
				EXPECT_EQ(pOffer->MosaicId.unwrap(), test::GetUint64(doc, "mosaicId"));
				EXPECT_EQ(pOffer->OfferType, static_cast<model::OfferType>(test::GetUint8(doc, "offerType")));
			}
		}

		template<typename TTraits>
		void AssertCanMapRemoveExchangeOfferTransaction(std::initializer_list<model::OfferMosaic> offers) {
			// Arrange:
			auto pTransaction = test::CreateExchangeTransaction<typename TTraits::TransactionType, model::OfferMosaic>(offers);
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			mappers::bson_stream::document builder;
			pPlugin->streamTransaction(builder, *pTransaction);
			auto view = builder.view();

			// Assert:
			EXPECT_EQ(1u, test::GetFieldCount(view));
			AssertEqualNonInheritedData(*pTransaction, view);
		}
	}

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_Remove_Exchange_Offer)

	PLUGIN_TEST(CanMapBuyRemoveExchangeOfferTransactionWithoutOffers) {
		// Assert:
		AssertCanMapRemoveExchangeOfferTransaction<TTraits>({});
	}

	PLUGIN_TEST(CanMapBuyRemoveExchangeOfferTransactionWithOffers) {
		// Assert:
		AssertCanMapRemoveExchangeOfferTransaction<TTraits>({
			model::OfferMosaic{test::GenerateRandomValue<UnresolvedMosaicId>(), model::OfferType::Buy},
			model::OfferMosaic{test::GenerateRandomValue<UnresolvedMosaicId>(), model::OfferType::Sell},
			model::OfferMosaic{test::GenerateRandomValue<UnresolvedMosaicId>(), model::OfferType::Buy},
			model::OfferMosaic{test::GenerateRandomValue<UnresolvedMosaicId>(), model::OfferType::Sell},
			model::OfferMosaic{test::GenerateRandomValue<UnresolvedMosaicId>(), model::OfferType::Buy},
		});
	}
}}}
