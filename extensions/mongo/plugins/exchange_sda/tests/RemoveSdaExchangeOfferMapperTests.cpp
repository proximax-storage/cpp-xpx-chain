/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/RemoveSdaExchangeOfferMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "plugins/txes/exchange_sda/src/model/RemoveSdaExchangeOfferTransaction.h"
#include "plugins/txes/exchange_sda/tests/test/SdaExchangeTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS RemoveSdaExchangeOfferMapperTests

    namespace {
        DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(RemoveSdaExchangeOffer,)

        template<typename TTransaction>
        void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
            auto iter = dbTransaction["offers"].get_array().value.cbegin();
            const model::SdaOfferMosaic* pOffer = transaction.SdaOffersPtr();
            for (uint8_t i = 0; i < transaction.SdaOfferCount; ++i, ++pOffer, ++iter) {
                const auto& doc = iter->get_document().view();
                EXPECT_EQ(pOffer->MosaicIdGive.unwrap(), test::GetUint64(doc, "mosaicIdGive"));
                EXPECT_EQ(pOffer->MosaicIdGet.unwrap(), test::GetUint64(doc, "mosaicIdGet"));
            }
        }

        template<typename TTraits>
        void AssertCanMapRemoveSdaExchangeOfferTransaction(std::initializer_list<model::SdaOfferMosaic> offers) {
            // Arrange:
            auto pTransaction = test::CreateSdaExchangeOfferTransaction<typename TTraits::TransactionType, model::SdaOfferMosaic>(offers);
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

    DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_Remove_Sda_Exchange_Offer)

    PLUGIN_TEST(CanMapRemoveSdaExchangeOfferTransactionWithoutOffers) {
        // Assert:
        AssertCanMapRemoveSdaExchangeOfferTransaction<TTraits>({});
    }

    PLUGIN_TEST(CanMapRemoveSdaExchangeOfferTransactionWithOffers) {
        // Assert:
        AssertCanMapRemoveSdaExchangeOfferTransaction<TTraits>({
            model::SdaOfferMosaic{test::GenerateRandomValue<UnresolvedMosaicId>(), test::GenerateRandomValue<UnresolvedMosaicId>()},
            model::SdaOfferMosaic{test::GenerateRandomValue<UnresolvedMosaicId>(), test::GenerateRandomValue<UnresolvedMosaicId>()},
            model::SdaOfferMosaic{test::GenerateRandomValue<UnresolvedMosaicId>(), test::GenerateRandomValue<UnresolvedMosaicId>()},
            model::SdaOfferMosaic{test::GenerateRandomValue<UnresolvedMosaicId>(), test::GenerateRandomValue<UnresolvedMosaicId>()},
            model::SdaOfferMosaic{test::GenerateRandomValue<UnresolvedMosaicId>(), test::GenerateRandomValue<UnresolvedMosaicId>()},
        });
    }
}}}
