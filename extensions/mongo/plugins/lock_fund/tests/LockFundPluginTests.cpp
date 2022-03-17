/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/tests/test/MongoPluginTestUtils.h"
#include "plugins/txes/lock_fund/src/model/LockFundEntityType.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct MongoPropertyPluginTraits {
		public:
			static constexpr auto RegisterSubsystem = RegisterMongoSubsystem;

			static std::vector<model::EntityType> GetTransactionTypes() {
				return {
					model::Entity_Type_Lock_Fund_Cancel_Unlock,
					model::Entity_Type_Lock_Fund_Transfer
				};
			}

			static std::vector<model::ReceiptType> GetReceiptTypes() {
				return {};
			}

			static std::string GetStorageName() {
				return "{ LockFundCache }";
			}
		};
	}

	DEFINE_MONGO_PLUGIN_TESTS(MongoPropertyPluginTests, MongoPropertyPluginTraits)
}}}
