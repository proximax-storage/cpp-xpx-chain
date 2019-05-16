/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/tests/test/MongoPluginTestUtils.h"
#include "plugins/txes/metadata/src/model/MetadataEntityType.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

	namespace {
        struct MongoMetadataPluginTraits {
            static constexpr auto RegisterSubsystem = RegisterMongoSubsystem;

            static std::vector<model::EntityType> GetTransactionTypes() {
                return {
                    model::Entity_Type_Address_Metadata,
                    model::Entity_Type_Mosaic_Metadata,
                    model::Entity_Type_Namespace_Metadata
                };
            }

            static std::vector<model::ReceiptType> GetReceiptTypes() {
                return {};
            }

            static std::string GetStorageName() {
                return "{ MetadataCache }";
            }
        };
	}

	DEFINE_MONGO_PLUGIN_TESTS(MongoMetadataPluginTests, MongoMetadataPluginTraits)
}}}
