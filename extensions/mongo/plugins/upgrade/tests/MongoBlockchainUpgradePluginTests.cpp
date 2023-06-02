/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/tests/test/MongoPluginTestUtils.h"
#include "plugins/txes/upgrade/src/model/BlockchainUpgradeEntityType.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct MongoBlockchainUpgradePluginTraits {
		public:
			static constexpr auto RegisterSubsystem = RegisterMongoSubsystem;

			static std::vector<model::EntityType> GetTransactionTypes() {
				return { model::Entity_Type_Blockchain_Upgrade, model::Entity_Type_AccountV2_Upgrade };
			}

			static std::vector<model::ReceiptType> GetReceiptTypes() {
				return {};
			}

			static std::string GetStorageName() {
				return "{ BlockchainUpgradeCache }";
			}
		};
	}

	DEFINE_MONGO_PLUGIN_TESTS(MongoBlockchainUpgradePluginTests, MongoBlockchainUpgradePluginTraits)
}}}
