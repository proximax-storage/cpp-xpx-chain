/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/ServicePlugin.h"
#include "plugins/txes/service/src/model/ServiceEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"

namespace catapult { namespace plugins {

	namespace {
		struct ServicePluginTraits {
		public:
			template<typename TAction>
			static void RunTestAfterRegistration(TAction action) {
				// Arrange:
				auto config = model::NetworkConfiguration::Uninitialized();
				config.Plugins.emplace(PLUGIN_NAME(exchange), utils::ConfigurationBag({{
					"",
					{
						{ "enabled", "true" },
						{ "maxOfferDuration", "1000" },
						{ "longOfferKey", "B4F12E7C9F6946091E2CB8B6D3A12B50D17CCBBF646386EA27CE2946A7423DCF" },
					}
				}}));

				auto manager = test::CreatePluginManager(config);
				RegisterServiceSubsystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return {
					model::Entity_Type_PrepareDrive,
					model::Entity_Type_JoinToDrive,
					model::Entity_Type_DriveFileSystem,
					model::Entity_Type_FilesDeposit,
					model::Entity_Type_EndDrive,
					model::Entity_Type_DriveFilesReward,
					model::Entity_Type_Start_Drive_Verification,
					model::Entity_Type_End_Drive_Verification,
					model::Entity_Type_StartFileDownload,
					model::Entity_Type_EndFileDownload,
				};
			}

			static std::vector<std::string> GetCacheNames() {
				return { "DriveCache", "DownloadCache" };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return { ionet::PacketType::Drive_Infos, ionet::PacketType::Download_Infos };
			}

			static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
				return { ionet::PacketType::Drive_State_Path, ionet::PacketType::Download_State_Path };
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return { "DRIVE C", "DOWNLOAD C" };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return {
					"ServicePluginConfigValidator",
					"FailedBlockHashesValidator",
					"PrepareDriveArgumentsValidator",
				};
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return {
					"DrivePermittedOperationValidator",
					"StartDriveVerificationValidator",
					"EndDriveVerificationValidator",
					"DriveValidator",
					"PrepareDrivePermissionValidator",
					"DriveFileSystemValidator",
					"MaxFilesOnDriveValidator",
					"JoinToDriveValidator",
					"FilesDepositValidator",
					"EndDriveValidator",
					"DriveFilesRewardValidator",
					"StartFileDownloadValidator",
					"EndFileDownloadValidator",
					"ExchangeValidator",
				};
			}

			static std::vector<std::string> GetObserverNames() {
				return {
					"DriveVerificationPaymentObserver",
					"EndBillingObserver",
					"DriveCacheBlockPruningObserver",
					"DownloadCacheTouchObserver",
					"DownloadCachePruningObserver",
					"StartBillingObserver",
					"PrepareDriveObserver",
					"DriveFileSystemObserver",
					"JoinToDriveObserver",
					"FilesDepositObserver",
					"EndDriveObserver",
					"DriveFilesRewardObserver",
					"StartFileDownloadObserver",
					"EndFileDownloadObserver",
				};
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return GetObserverNames();
			}
		};
	}

	DEFINE_PLUGIN_TESTS(ServicePluginTests, ServicePluginTraits)
}}
