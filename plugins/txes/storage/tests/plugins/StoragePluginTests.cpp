/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/StoragePlugin.h"
#include "plugins/txes/storage/src/model/StorageEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"

namespace catapult { namespace plugins {

	namespace {
		struct StoragePluginTraits {
		public:
			template<typename TAction>
			static void RunTestAfterRegistration(TAction action) {
				// Arrange:
				auto config = model::NetworkConfiguration::Uninitialized();
				config.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(1);
				config.Plugins.emplace(PLUGIN_NAME(locksecret), utils::ConfigurationBag({{
					"",
					{
						{ "enabled", "true" },
					}
				}}));

				auto manager = test::CreatePluginManager(config);
				RegisterStorageSubsystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return {
					model::Entity_Type_PrepareBcDrive,
					model::Entity_Type_DataModification,
					model::Entity_Type_Download,
					model::Entity_Type_DataModificationApproval,
					model::Entity_Type_DataModificationCancel,
					model::Entity_Type_ReplicatorOnboarding,
					model::Entity_Type_DriveClosure,
					model::Entity_Type_ReplicatorOffboarding,
					model::Entity_Type_FinishDownload,
					model::Entity_Type_DownloadPayment,
					model::Entity_Type_StoragePayment,
					model::Entity_Type_DataModificationSingleApproval,
					model::Entity_Type_VerificationPayment,
					model::Entity_Type_DownloadApproval,
					model::Entity_Type_EndDriveVerification,
					model::Entity_Type_ReplicatorsCleanup,
				};
			}

			static std::vector<std::string> GetCacheNames() {
				return { "BcDriveCache", "DownloadChannelCache", "ReplicatorCache", "QueueCache", "PriorityQueueCache", "BootKeyReplicatorCache" };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return { ionet::PacketType::BcDrive_Infos, ionet::PacketType::DownloadChannel_Infos,
						 ionet::PacketType::Replicator_Infos, ionet::PacketType::Queue_Infos,
						 ionet::PacketType::PriorityQueue_Infos, ionet::PacketType::BootKeyReplicator_Infos };
			}

			static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
				return { ionet::PacketType::BcDrive_State_Path, ionet::PacketType::DownloadChannel_State_Path,
						 ionet::PacketType::Replicator_State_Path, ionet::PacketType::Queue_State_Path,
						 ionet::PacketType::PriorityQueue_State_Path, ionet::PacketType::BootKeyReplicator_State_Path };
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return { "BC DRIVE C", "BOOTKEYREP C", "DOWNLOAD CH C", "QUEUE C", "PR QUEUE C", "REPLICATOR C" };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return {
					"StoragePluginConfigValidator",
				};
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return {
					"ServiceUnitTransferValidator",
					"DataModificationValidator",
					"DownloadChannelValidator",
					"PrepareDriveValidator",
					"DataModificationApprovalValidator",
					"DataModificationCancelValidator",
					"ReplicatorOnboardingV1Validator",
					"ReplicatorOffboardingValidator",
					"FinishDownloadValidator",
					"DownloadPaymentValidator",
					"StoragePaymentValidator",
					"DataModificationSingleApprovalValidator",
					"VerificationPaymentValidator",
					"OpinionValidator",
					"DownloadApprovalValidator",
					"DownloadChannelRefundValidator",
					"DriveClosureValidator",
					"DataModificationApprovalDownloadWorkValidator",
					"DataModificationApprovalUploadWorkValidator",
					"DataModificationApprovalRefundValidator",
					"StreamStartValidator",
					"StreamFinishValidator",
					"StreamPaymentValidator",
					"EndDriveVerificationValidator",
					"OwnerManagementProhibitionValidator",
					"ReplicatorNodeBootKeyValidator",
					"ReplicatorsCleanupV1Validator",
					"ReplicatorOnboardingV2Validator",
					"ReplicatorsCleanupV2Validator",
				};
			}

			static std::vector<std::string> GetObserverNames() {
				return {
					"StartDriveVerificationObserver",
					"PeriodicStoragePaymentObserver",
					"PeriodicDownloadChannelPaymentObserver",
					"DataModificationObserver",
					"DownloadChannelObserver",
					"PrepareDriveObserver",
					"DataModificationApprovalObserver",
					"DataModificationCancelObserver",
					"ReplicatorOnboardingV1Observer",
					"ReplicatorOffboardingObserver",
					"FinishDownloadObserver",
					"DownloadPaymentObserver",
					"DataModificationSingleApprovalObserver",
					"DownloadApprovalObserver",
					"DownloadApprovalPaymentObserver",
					"DownloadChannelRefundObserver",
					"DriveClosureObserver",
					"DataModificationApprovalDownloadWorkObserver",
					"DataModificationApprovalUploadWorkObserver",
					"DataModificationApprovalRefundObserver",
					"StreamStartObserver",
					"StreamFinishObserver",
					"StreamPaymentObserver",
					"EndDriveVerificationObserver",
					"OwnerManagementProhibitionObserver",
					"ReplicatorNodeBootKeyObserver",
					"ReplicatorsCleanupV1Observer",
					"ReplicatorOnboardingV2Observer",
					"ReplicatorsCleanupV2Observer",
				};
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return GetObserverNames();
			}
		};
	}

	DEFINE_PLUGIN_TESTS(StoragePluginTests, StoragePluginTraits)
}}
