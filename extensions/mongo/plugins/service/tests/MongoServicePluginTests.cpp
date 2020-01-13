/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/tests/test/MongoPluginTestUtils.h"
#include "plugins/txes/service/src/model/ServiceEntityType.h"
#include "plugins/txes/service/src/model/ServiceReceiptType.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct MongoServicePluginTraits {
		public:
			static constexpr auto RegisterSubsystem = RegisterMongoSubsystem;

			static std::vector<model::EntityType> GetTransactionTypes() {
				return {
					model::Entity_Type_PrepareDrive,
					model::Entity_Type_DriveFileSystem,
					model::Entity_Type_JoinToDrive,
					model::Entity_Type_FilesDeposit,
					model::Entity_Type_EndDrive,
					model::Entity_Type_DriveFilesReward,
					model::Entity_Type_Start_Drive_Verification,
					model::Entity_Type_End_Drive_Verification,
					model::Entity_Type_StartFileDownload,
					model::Entity_Type_EndFileDownload,
				};
			}

			static std::vector<model::ReceiptType> GetReceiptTypes() {
				return { model::Receipt_Type_Drive_State };
			}

			static std::string GetStorageName() {
				return "{ DriveCache }";
			}
		};
	}

	DEFINE_MONGO_PLUGIN_TESTS(MongoServicePluginTests, MongoServicePluginTraits)
}}}
