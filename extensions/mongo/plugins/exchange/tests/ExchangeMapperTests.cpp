/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/ExchangeMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "plugins/txes/exchange/src/model/ExchangeTransaction.h"
#include "plugins/txes/exchange/tests/test/ExchangeTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS ExchangeMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(Exchange,)

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			auto iter = dbTransaction["offers"].get_array().value.cbegin();
			const model::MatchedOffer* pOffer = transaction.OffersPtr();
			for (uint8_t i = 0; i < transaction.OfferCount; ++i, ++pOffer, ++iter) {
				const auto& doc = iter->get_document().view();
				EXPECT_EQ(pOffer->Mosaic.MosaicId.unwrap(), test::GetUint64(doc, "mosaicId"));
				EXPECT_EQ(pOffer->Mosaic.Amount.unwrap(), test::GetUint64(doc, "mosaicAmount"));
				EXPECT_EQ(pOffer->Cost.unwrap(), test::GetUint64(doc, "cost"));
				EXPECT_EQ(pOffer->Type, static_cast<model::OfferType>(test::GetUint8(doc, "type")));
				Key owner;
				mongo::mappers::DbBinaryToModelArray(owner, doc["owner"].get_binary());
				EXPECT_EQ(pOffer->Owner, owner);
			}
		}

		template<typename TTraits>
		void AssertCanMapExchangeTransaction(std::initializer_list<model::MatchedOffer> offers) {
			// Arrange:
			auto pTransaction = test::CreateExchangeTransaction<typename TTraits::TransactionType, model::MatchedOffer>(offers);
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			mappers::bson_stream::document builder;
			pPlugin->streamTransaction(builder, *pTransaction);
			auto view = builder.view();

			// Assert:
			EXPECT_EQ(1u, test::GetFieldCount(view));
			AssertEqualNonInheritedData(*pTransaction, view);
		}

		model::MatchedOffer GenerateOffer(model::OfferType offerType) {
			return model::MatchedOffer{
				model::Offer{
					model::UnresolvedMosaic{test::GenerateRandomValue<UnresolvedMosaicId>(), test::GenerateRandomValue<Amount>()},
					test::GenerateRandomValue<Amount>(),
					offerType,
				},
				test::GenerateRandomByteArray<Key>()
			};
		}
	}

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_Exchange)

	PLUGIN_TEST(CanMapBuyExchangeTransactionWithoutOffers) {
		// Assert:
		AssertCanMapExchangeTransaction<TTraits>({});
	}

	PLUGIN_TEST(CanMapBuyExchangeTransactionWithOffers) {
		// Assert:
		AssertCanMapExchangeTransaction<TTraits>({
			GenerateOffer(model::OfferType::Buy),
			GenerateOffer(model::OfferType::Sell),
			GenerateOffer(model::OfferType::Buy),
			GenerateOffer(model::OfferType::Sell),
			GenerateOffer(model::OfferType::Buy),
		});
	}
}}}
