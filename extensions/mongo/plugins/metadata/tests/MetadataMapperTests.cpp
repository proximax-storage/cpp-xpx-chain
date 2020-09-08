/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/MetadataMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "plugins/txes/metadata/src/model/AddressMetadataTransaction.h"
#include "plugins/txes/metadata/src/model/MosaicMetadataTransaction.h"
#include "plugins/txes/metadata/src/model/NamespaceMetadataTransaction.h"
#include "plugins/txes/metadata/src/state/MetadataUtils.h"
#include "plugins/txes/metadata/tests/test/MetadataTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MetadataMapperTests

    template<typename TTraits>
    class MetadataMapperTests {
    public:
        static void AssertCanMapMetadataTransactionWithoutFields() {
            // Assert:
            AssertCanMapMetadataTransaction({});
        }

        static void AssertCanMapMetadataTransactionWithSingleField() {
            // Assert:
            AssertCanMapMetadataTransaction({
                test::CreateModification(model::MetadataModificationType::Add, 1, 2).get()
            });
        }

        static void AssertCanMapMetadataTransactionWithMultipleFields() {
            // Assert:
            AssertCanMapMetadataTransaction({
                test::CreateModification(model::MetadataModificationType::Add, 1, 2).get(),
                test::CreateModification(model::MetadataModificationType::Add, 3, 4).get(),
                test::CreateModification(model::MetadataModificationType::Add, 5, 6).get()
            });
        }

    private:
        static void AssertMetadataTransaction(const typename TTraits::TransactionType& transaction, const bsoncxx::document::view& dbTransaction) {
            EXPECT_EQ(transaction.MetadataType, model::MetadataType(test::GetUint8(dbTransaction, "metadataType")));
            std::vector<uint8_t> metadataId;
            mongo::mappers::DbBinaryToStdContainer(metadataId, dbTransaction["metadataId"].get_binary());
            EXPECT_EQ(state::ToVector(transaction.MetadataId), metadataId);

            auto dbModifications = dbTransaction["modifications"].get_array().value;
            auto modifications = transaction.Transactions();
            ASSERT_EQ(std::distance(modifications.begin(), modifications.end()), test::GetFieldCount(dbModifications));
            auto iter = dbModifications.cbegin();
            for (const auto& modification : modifications) {
                EXPECT_EQ(
                        modification.ModificationType,
                        static_cast<model::MetadataModificationType>(test::GetUint8(iter->get_document().view(), "modificationType")));
                EXPECT_EQ(
                        std::string(modification.KeyPtr(), modification.KeySize),
                        iter->get_document().view()["key"].get_utf8().value.to_string());
                EXPECT_EQ(
                        std::string(modification.ValuePtr(), modification.ValueSize),
                        iter->get_document().view()["value"].get_utf8().value.to_string());
                ++iter;
            }
        }

        static void AssertCanMapMetadataTransaction(std::initializer_list<model::MetadataModification*> modifications) {
            // Arrange:
            auto pTransaction = test::CreateTransaction<typename TTraits::TransactionType>(modifications);
            auto pPlugin = TTraits::CreatePlugin();

            // Act:
            mappers::bson_stream::document builder;
            pPlugin->streamTransaction(builder, *pTransaction);
            auto view = builder.view();

            // Assert:
            EXPECT_EQ(3u, test::GetFieldCount(view));
            AssertMetadataTransaction(*pTransaction, view);
        }
    };

#define MAKE_METADATA_MAPPER_TEST(TRAITS, PREFIX, TEST_NAME) \
	TEST(TEST_CLASS, TEST_NAME##_##PREFIX##_##Regular) { \
		MetadataMapperTests<PREFIX##Regular##TRAITS>::Assert##TEST_NAME(); \
	} \
	TEST(TEST_CLASS, TEST_NAME##_##PREFIX##_##Embedded) { \
		MetadataMapperTests<PREFIX##Embedded##TRAITS>::Assert##TEST_NAME(); \
	}

#define DEFINE_METADATA_MAPPER_TESTS_WITH_PREFIXED_TRAITS(TRAITS, PREFIX) \
	MAKE_METADATA_MAPPER_TEST(TRAITS, PREFIX, CanMapMetadataTransactionWithoutFields) \
	MAKE_METADATA_MAPPER_TEST(TRAITS, PREFIX, CanMapMetadataTransactionWithSingleField) \
	MAKE_METADATA_MAPPER_TEST(TRAITS, PREFIX, CanMapMetadataTransactionWithMultipleFields)

    namespace {
        DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(AddressMetadata, Address)
        DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(MosaicMetadata, Mosaic)
        DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(NamespaceMetadata, Namespace)
    }

    DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, Address, _Address, model::Entity_Type_Address_Metadata)
    DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, Mosaic, _Mosaic, model::Entity_Type_Mosaic_Metadata)
    DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, Namespace, _Namespace, model::Entity_Type_Namespace_Metadata)

    DEFINE_METADATA_MAPPER_TESTS_WITH_PREFIXED_TRAITS(Traits, Address)
    DEFINE_METADATA_MAPPER_TESTS_WITH_PREFIXED_TRAITS(Traits, Mosaic)
    DEFINE_METADATA_MAPPER_TESTS_WITH_PREFIXED_TRAITS(Traits, Namespace)
}}}
