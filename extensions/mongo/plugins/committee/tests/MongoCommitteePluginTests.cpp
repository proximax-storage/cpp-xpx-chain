/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/tests/test/MongoPluginTestUtils.h"
#include "plugins/txes/committee/src/model/CommitteeEntityType.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct MongoCommitteePluginTraits {
		public:
			static constexpr auto RegisterSubsystem = RegisterMongoSubsystem;

			static std::vector<model::EntityType> GetTransactionTypes() {
				return {
					model::Entity_Type_AddHarvester,
					model::Entity_Type_RemoveHarvester,
				};
			}

			static std::vector<model::ReceiptType> GetReceiptTypes() {
				return {};
			}

			static std::string GetStorageName() {
				return "{ CommitteeCache }";
			}
		};
	}

	DEFINE_MONGO_PLUGIN_TESTS(MongoCommitteePluginTests, MongoCommitteePluginTraits)
}}}
