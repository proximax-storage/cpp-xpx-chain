/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/PluginUtils.h"
#include "src/plugins/SuperContractPlugin.h"
#include "src/model/SuperContractEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

	namespace {
		struct SuperContractPluginTraits {
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
				RegisterSuperContractSubsystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return {
					model::Entity_Type_Deploy,
					model::Entity_Type_StartExecute,
					model::Entity_Type_EndExecute,
					model::Entity_Type_UploadFile,
					model::Entity_Type_Deactivate,
				};
			}

			static std::vector<std::string> GetCacheNames() {
				return { "SuperContractCache" };
			}

			static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
				return { ionet::PacketType::SuperContract_State_Path };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return { ionet::PacketType::SuperContract_Infos };
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return { "SC C" };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return {
					"SuperContractPluginConfigValidator",
					"AggregateTransactionValidator",
				};
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return {
					"StartExecuteValidator",
					"EndExecuteValidator",
					"SuperContractValidator",
					"DeployValidator",
					"DriveFileSystemValidator",
					"EndOperationTransactionValidator",
					"DeactivateValidator",
				};
			}

			static std::vector<std::string> GetObserverNames() {
				return {
					"DeployObserver",
					"StartExecuteObserver",
					"EndExecuteObserver",
					"AggregateTransactionHashObserver",
					"DeactivateObserver",
					"EndDriveObserver",
				};
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return GetObserverNames();
			}
		};
	}

	DEFINE_PLUGIN_TESTS(SuperContractPluginTests, SuperContractPluginTraits)
}}
