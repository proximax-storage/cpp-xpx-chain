/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataCacheTraits.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"


namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MongoAddressMetadataCacheStorageTests

    namespace {
        struct AddressMetadataEntryTraits {
            static constexpr auto MetadataType = model::MetadataType::Address;

            static std::vector<uint8_t> GenerateEntryBuffer(model::NetworkIdentifier network_Id) {
                auto pubKey = test::GenerateRandomByteArray<Key>();
                auto address = model::PublicKeyToAddress(pubKey, network_Id);
                std::vector<uint8_t> buffer{address.size()};
                std::copy(address.begin(), address.end(), std::back_inserter(buffer));

                return buffer;
            }
        };
    }

	DEFINE_FLAT_CACHE_STORAGE_TESTS(test::MetadataCacheTraits<AddressMetadataEntryTraits>,)
}}}
