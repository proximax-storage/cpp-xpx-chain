/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "src/mappers/MetadataEntryMapper.h"
#include "tests/test/MetadataMapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MetadataEntryMapperTests

	// region ToDbModel

	namespace {
        constexpr auto Network_Identifier = model::NetworkIdentifier::Private_Test;

        state::MetadataEntry CreateMetadataEntry(int fieldCount, model::MetadataType type) {
            std::vector<uint8_t> buffer;
            switch (type) {
                case model::MetadataType::Address: {
                    auto pubKey = test::GenerateRandomData<Key_Size>();
                    auto address = model::PublicKeyToAddress(pubKey, Network_Identifier);
                    std::copy(address.begin(), address.end(), std::back_inserter(buffer));
                    break;
                }

                case model::MetadataType::MosaicId: {
                    auto data = test::GenerateRandomData<sizeof(UnresolvedMosaicId)>();
                    buffer = std::vector<uint8_t>(data.cbegin(), data.cend());
                    break;
                }

                case model::MetadataType::NamespaceId: {
                    auto data = test::GenerateRandomData<sizeof(NamespaceId)>();
                    buffer = std::vector<uint8_t>(data.cbegin(), data.cend());
                    break;
                }
            }

            auto entry = state::MetadataEntry{ buffer, type };

            for (int i = 0; i < fieldCount; ++i) {
                entry.fields().emplace_back(state::MetadataField{
                        test::GenerateRandomString(15), test::GenerateRandomString(10), Height{0}});
            }

            return entry;
        }

        void AssertCanMapMetadataEntry(int fieldCount, model::MetadataType type) {
            // Arrange:
            auto entry = CreateMetadataEntry(fieldCount, type);

            // Act:
            auto document = ToDbModel(entry);
            auto documentView = document.view();

            // Assert:
            EXPECT_EQ(1u, test::GetFieldCount(documentView));

            auto metadataView = documentView["metadata"].get_document().view();
            EXPECT_EQ(3u, test::GetFieldCount(metadataView));
            test::AssertEqualMetadataData(entry, metadataView);
        }
	}

	TEST(TEST_CLASS, CanMapMetadataEntryWithoutFields_ModelToDbModel_Address) {
        // Assert:
        AssertCanMapMetadataEntry(0, model::MetadataType::Address);
	}

	TEST(TEST_CLASS, CanMapMetadataEntryWithFields_ModelToDbModel_Address) {
        // Assert:
        AssertCanMapMetadataEntry(5, model::MetadataType::Address);
	}

	TEST(TEST_CLASS, CanMapMetadataEntryWithoutFields_ModelToDbModel_MosaicId) {
        // Assert:
        AssertCanMapMetadataEntry(0, model::MetadataType::MosaicId);
	}

	TEST(TEST_CLASS, CanMapMetadataEntryWithFields_ModelToDbModel_MosaicId) {
        // Assert:
        AssertCanMapMetadataEntry(5, model::MetadataType::MosaicId);
	}

	TEST(TEST_CLASS, CanMapMetadataEntryWithoutFields_ModelToDbModel_NamespaceId) {
        // Assert:
        AssertCanMapMetadataEntry(0, model::MetadataType::NamespaceId);
	}

	TEST(TEST_CLASS, CanMapMetadataEntryWithFields_ModelToDbModel_NamespaceId) {
        // Assert:
        AssertCanMapMetadataEntry(5, model::MetadataType::NamespaceId);
	}

	// endregion

	// region ToMetadataEntry

	namespace {
        bsoncxx::document::value CreateDbMetadataEntry(int fieldCount, model::MetadataType type) {
            return ToDbModel(CreateMetadataEntry(fieldCount, type));
        }

        void AssertCanMapDbMetadataEntry(int fieldCount, model::MetadataType type) {
            // Arrange:
            auto dbMetadataEntry = CreateDbMetadataEntry(fieldCount, type);

            // Act:
            auto entry = ToMetadataEntry(dbMetadataEntry);

            // Assert:
            auto view = dbMetadataEntry.view();
            EXPECT_EQ(1u, test::GetFieldCount(view));

            auto metadataView = view["metadata"].get_document().view();
            EXPECT_EQ(3u, test::GetFieldCount(metadataView));
            test::AssertEqualMetadataData(entry, metadataView);
        }
	}

	TEST(TEST_CLASS, CanMapMetadataEntryWithoutFields_DbModelToModel_Address) {
        // Assert:
        AssertCanMapDbMetadataEntry(0, model::MetadataType::Address);
	}

	TEST(TEST_CLASS, CanMapMetadataEntryWithFields_DbModelToModel_Address) {
        // Assert:
        AssertCanMapDbMetadataEntry(5, model::MetadataType::Address);
	}

	TEST(TEST_CLASS, CanMapMetadataEntryWithoutFields_DbModelToModel_MosaicId) {
        // Assert:
        AssertCanMapDbMetadataEntry(0, model::MetadataType::MosaicId);
	}

	TEST(TEST_CLASS, CanMapMetadataEntryWithFields_DbModelToModel_MosaicId) {
        // Assert:
        AssertCanMapDbMetadataEntry(5, model::MetadataType::MosaicId);
	}

	TEST(TEST_CLASS, CanMapMetadataEntryWithoutFields_DbModelToModel_NamespaceId) {
        // Assert:
        AssertCanMapDbMetadataEntry(0, model::MetadataType::NamespaceId);
	}

	TEST(TEST_CLASS, CanMapMetadataEntryWithFields_DbModelToModel_NamespaceId) {
        // Assert:
        AssertCanMapDbMetadataEntry(5, model::MetadataType::NamespaceId);
	}

	// endregion
}}}
