/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/tests/test/MongoPluginTestUtils.h"
#include "plugins/txes/contract/src/model/ContractEntityType.h"

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct MongoContractPluginTraits {
		public:
			static constexpr auto RegisterSubsystem = RegisterMongoSubsystem;

			static std::vector<model::EntityType> GetTransactionTypes() {
				return { model::Entity_Type_Modify_Contract };
			}

			static std::vector<model::ReceiptType> GetReceiptTypes() {
				return {};
			}

			static std::string GetStorageName() {
				return "{ ReputationCache, ContractCache }";
			}
		};
	}

	DEFINE_MONGO_PLUGIN_TESTS(MongoContractPluginTests, MongoContractPluginTraits)
}}}
