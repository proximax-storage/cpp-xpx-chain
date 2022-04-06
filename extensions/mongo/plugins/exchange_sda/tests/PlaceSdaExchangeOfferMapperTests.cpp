/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/PlaceSdaExchangeOfferMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "plugins/txes/exchange_sda/src/model/PlaceSdaExchangeOfferTransaction.h"
#include "plugins/txes/exchange_sda/tests/test/SdaExchangeTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS PlaceSdaExchangeOfferMapperTests

    namespace {
        DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(PlaceSdaExchangeOffer,)

        template<typename TTransaction>
        void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
            auto iter = dbTransaction["offers"].get_array().value.cbegin();
            const model::SdaOfferWithOwnerAndDuration* pOffer = transaction.SdaOffersPtr();
            for (uint8_t i = 0; i < transaction.SdaOfferCount; ++i, ++pOffer, ++iter) {
                const auto& doc = iter->get_document().view();
                EXPECT_EQ(pOffer->MosaicGive.MosaicId.unwrap(), test::GetUint64(doc, "mosaicIdGive"));
                EXPECT_EQ(pOffer->MosaicGive.Amount.unwrap(), test::GetUint64(doc, "mosaicAmountGive"));
                EXPECT_EQ(pOffer->MosaicGet.MosaicId.unwrap(), test::GetUint64(doc, "mosaicIdGet"));
                EXPECT_EQ(pOffer->MosaicGet.Amount.unwrap(), test::GetUint64(doc, "mosaicAmountGet"));
                EXPECT_EQ(pOffer->Owner.unwrap(), test::GetKeyValue(doc, "owner"));
                EXPECT_EQ(pOffer->Duration.unwrap(), test::GetUint64(doc, "duration"));
            }
        }

        template<typename TTraits>
        void AssertCanMapPlaceSdaExchangeOfferTransaction(std::initializer_list<model::SdaOfferWithOwnerAndDuration> offers) {
            // Arrange:
            auto pTransaction = test::CreatePlaceSdaExchangeOfferTransaction<typename TTraits::TransactionType, model::SdaOfferWithOwnerAndDuration>(offers);
            auto pPlugin = TTraits::CreatePlugin();

            // Act:
            mappers::bson_stream::document builder;
            pPlugin->streamTransaction(builder, *pTransaction);
            auto view = builder.view();

            // Assert:
            EXPECT_EQ(1u, test::GetFieldCount(view));
            AssertEqualNonInheritedData(*pTransaction, view);
        }

        model::SdaOfferWithOwnerAndDuration GenerateSdaOffer(Key owner) {
            return model::SdaOfferWithOwnerAndDuration{
                model::UnresolvedMosaic{test::GenerateRandomValue<UnresolvedMosaicId>(), test::GenerateRandomValue<Amount>()},
                model::UnresolvedMosaic{test::GenerateRandomValue<UnresolvedMosaicId>(), test::GenerateRandomValue<Amount>()},
                owner,   
                test::GenerateRandomValue<BlockDuration>()
            };
        }
    }

    DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_Place_Sda_Exchange_Offer)

    PLUGIN_TEST(CanMapPlaceSdaExchangeOfferTransactionWithoutOffers) {
        // Assert:
        AssertCanMapPlaceSdaExchangeOfferTransaction<TTraits>({});
    }

    PLUGIN_TEST(CanMapPlaceSdaExchangeOfferTransactionWithOffers) {
        // Arrange:
        Key& owner = test::GenerateRandomByteArray<Key>();

        // Assert:
        AssertCanMapPlaceSdaExchangeOfferTransaction<TTraits>({
            GenerateSdaOffer(owner),
            GenerateSdaOffer(owner),
            GenerateSdaOffer(owner),
            GenerateSdaOffer(owner),
            GenerateSdaOffer(owner),
        });
    }
}}}
