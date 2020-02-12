/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/tests/test/MongoPluginTestUtils.h"
#include "plugins/txes/operation/src/model/OperationEntityType.h"
#include "plugins/txes/operation/src/model/OperationReceiptType.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct MongoOperationPluginTraits {
		public:
			static constexpr auto RegisterSubsystem = RegisterMongoSubsystem;

			static std::vector<model::EntityType> GetTransactionTypes() {
				return {
					model::Entity_Type_OperationIdentify,
					model::Entity_Type_StartOperation,
					model::Entity_Type_EndOperation,
				};
			}

			static std::vector<model::ReceiptType> GetReceiptTypes() {
				return {};
			}

			static std::string GetStorageName() {
				return "{ OperationCache }";
			}
		};
	}

	DEFINE_MONGO_PLUGIN_TESTS(MongoOperationPluginTests, MongoOperationPluginTraits)
}}}
