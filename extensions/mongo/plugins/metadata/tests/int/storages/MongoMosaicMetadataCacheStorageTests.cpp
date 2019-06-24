/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataCacheTraits.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"
#include "tests/TestHarness.h"


namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MongoMosaicMetadataCacheStorageTests

    namespace {
        struct MosaicMetadataEntryTraits {
            static constexpr auto MetadataType = model::MetadataType::MosaicId;

            static std::vector<uint8_t> GenerateEntryBuffer(model::NetworkIdentifier) {
                return test::GenerateRandomVector(sizeof(UnresolvedMosaicId));
            }
        };
    }

    DEFINE_FLAT_CACHE_STORAGE_TESTS(test::MetadataCacheTraits<MosaicMetadataEntryTraits>,)
}}}
