/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataCacheTraits.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MongoNamespaceMetadataCacheStorageTests

    namespace {
        struct NamespaceMetadataEntryTraits {
            static constexpr auto MetadataType = model::MetadataV1Type::NamespaceId;

            static std::vector<uint8_t> GenerateEntryBuffer(model::NetworkIdentifier) {
				return test::GenerateRandomVector(sizeof(NamespaceId));
            }
        };
    }

	DEFINE_FLAT_CACHE_STORAGE_TESTS(test::MetadataCacheTraits<NamespaceMetadataEntryTraits>,)
}}}
