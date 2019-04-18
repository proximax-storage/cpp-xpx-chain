/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "catapult/model/NetworkInfo.h"
#include "src/state/MetadataEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS MetadataEntryTests

    namespace {
        constexpr auto Network_Identifier = model::NetworkIdentifier::Private_Test;
    }

    TEST(TEST_CLASS, CanCreateEmptyMetadataEntry) {
        // Act:
        auto entry = MetadataEntry{};

        // Assert:
        EXPECT_TRUE(entry.fields().empty());
        EXPECT_EQ(model::MetadataType{0}, entry.type());
        EXPECT_TRUE(entry.raw().empty());
    }

    TEST(TEST_CLASS, CanCreateAddressMetadataEntry) {
        // Arrange:
        auto pubKey = test::GenerateRandomData<Key_Size>();
        auto address = model::PublicKeyToAddress(pubKey, Network_Identifier);
        std::vector<uint8_t> buffer{address.size()};
        std::copy(address.begin(), address.end(), std::back_inserter(buffer));
        auto type = model::MetadataType::Address;
        auto hash = GetHash(buffer, type);

        // Act:
        auto entry = MetadataEntry{buffer, type};

        // Assert:
        EXPECT_EQ(hash, entry.metadataId());
        EXPECT_TRUE(entry.fields().empty());
        EXPECT_EQ(type, entry.type());
        EXPECT_EQ(buffer, entry.raw());
    }

    TEST(TEST_CLASS, CanCreateMosaicMetadataEntry) {
        // Arrange:
        std::vector<uint8_t> buffer{sizeof(MosaicId)};
        auto mosaicId = MosaicId{0x1234567890ABCDEF};
        auto pData = reinterpret_cast<uint8_t*>(&mosaicId);
        std::copy(pData, pData + sizeof(MosaicId), std::back_inserter(buffer));
        auto type = model::MetadataType::MosaicId;
        auto hash = GetHash(buffer, type);

        // Act:
        auto entry = MetadataEntry{buffer, type};

        // Assert:
        EXPECT_EQ(hash, entry.metadataId());
        EXPECT_TRUE(entry.fields().empty());
        EXPECT_EQ(type, entry.type());
        EXPECT_EQ(buffer, entry.raw());
    }

    TEST(TEST_CLASS, CanCreateNamespaceMetadataEntry) {
        // Arrange:
        std::vector<uint8_t> buffer{sizeof(NamespaceId)};
        auto namespaceId = NamespaceId{0x1234567890ABCDEF};
        auto pData = reinterpret_cast<uint8_t*>(&namespaceId);
        std::copy(pData, pData + sizeof(NamespaceId), std::back_inserter(buffer));
        auto type = model::MetadataType::NamespaceId;
        auto hash = GetHash(buffer, type);

        // Act:
        auto entry = MetadataEntry{buffer, type};

        // Assert:
        EXPECT_EQ(hash, entry.metadataId());
        EXPECT_TRUE(entry.fields().empty());
        EXPECT_EQ(type, entry.type());
        EXPECT_EQ(buffer, entry.raw());
    }
}}
