/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "mongo/src/CoreMongo.h"
#include "mongo/tests/test/MongoPluginTestUtils.h"

namespace catapult { namespace mongo {

	namespace {
		struct CoreMongoTraits {
		public:
			static constexpr auto RegisterSubsystem = RegisterCoreMongoSystem;

			static std::vector<model::EntityType> GetTransactionTypes() {
				return {};
			}

			static std::vector<model::ReceiptType> GetReceiptTypes() {
				return { model::Receipt_Type_Harvest_Fee, model::Receipt_Type_Inflation };
			}

			static std::string GetStorageName() {
				return "{ AccountStateCache }";
			}
		};
	}

	DEFINE_MONGO_PLUGIN_TESTS(CoreMongoTests, CoreMongoTraits)
}}
