/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/tests/test/MongoPluginTestUtils.h"
#include "plugins/txes/config/src/model/NetworkConfigEntityType.h"

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct MongoNetworkConfigPluginTraits {
		public:
			static constexpr auto RegisterSubsystem = RegisterMongoSubsystem;

			static std::vector<model::EntityType> GetTransactionTypes() {
				return { model::Entity_Type_Network_Config };
			}

			static std::vector<model::ReceiptType> GetReceiptTypes() {
				return {};
			}

			static std::string GetStorageName() {
				return "{ NetworkConfigCache }";
			}
		};
	}

	DEFINE_MONGO_PLUGIN_TESTS(MongoNetworkConfigPluginTests, MongoNetworkConfigPluginTraits)
}}}
