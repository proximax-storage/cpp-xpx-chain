/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/ExchangeOfferMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "plugins/txes/exchange/src/model/ExchangeOfferTransaction.h"
#include "plugins/txes/exchange/tests/test/ExchangeTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS ExchangeOfferMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(ExchangeOffer,)

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			auto iter = dbTransaction["offers"].get_array().value.cbegin();
			const model::OfferWithDuration* pOffer = transaction.OffersPtr();
			for (uint8_t i = 0; i < transaction.OfferCount; ++i, ++pOffer, ++iter) {
				const auto& doc = iter->get_document().view();
				EXPECT_EQ(pOffer->Mosaic.MosaicId.unwrap(), test::GetUint64(doc, "mosaicId"));
				EXPECT_EQ(pOffer->Mosaic.Amount.unwrap(), test::GetUint64(doc, "mosaicAmount"));
				EXPECT_EQ(pOffer->Cost.unwrap(), test::GetUint64(doc, "cost"));
				EXPECT_EQ(pOffer->Type, static_cast<model::OfferType>(test::GetUint8(doc, "type")));
				EXPECT_EQ(pOffer->Duration.unwrap(), test::GetUint64(doc, "duration"));
			}
		}

		template<typename TTraits>
		void AssertCanMapExchangeOfferTransaction(std::initializer_list<model::OfferWithDuration> offers) {
			// Arrange:
			auto pTransaction = test::CreateExchangeTransaction<typename TTraits::TransactionType, model::OfferWithDuration>(offers);
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			mappers::bson_stream::document builder;
			pPlugin->streamTransaction(builder, *pTransaction);
			auto view = builder.view();

			// Assert:
			EXPECT_EQ(1u, test::GetFieldCount(view));
			AssertEqualNonInheritedData(*pTransaction, view);
		}

		model::OfferWithDuration GenerateOffer(model::OfferType offerType) {
			return model::OfferWithDuration{
				model::Offer{
					model::UnresolvedMosaic{test::GenerateRandomValue<UnresolvedMosaicId>(), test::GenerateRandomValue<Amount>()},
					test::GenerateRandomValue<Amount>(),
					offerType,
				},
				test::GenerateRandomValue<BlockDuration>()
			};
		}
	}

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_Exchange_Offer)

	PLUGIN_TEST(CanMapBuyExchangeOfferTransactionWithoutOffers) {
		// Assert:
		AssertCanMapExchangeOfferTransaction<TTraits>({});
	}

	PLUGIN_TEST(CanMapBuyExchangeOfferTransactionWithOffers) {
		// Assert:
		AssertCanMapExchangeOfferTransaction<TTraits>({
			GenerateOffer(model::OfferType::Buy),
			GenerateOffer(model::OfferType::Sell),
			GenerateOffer(model::OfferType::Buy),
			GenerateOffer(model::OfferType::Sell),
			GenerateOffer(model::OfferType::Buy),
		});
	}
}}}
