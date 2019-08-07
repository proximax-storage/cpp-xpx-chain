/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/PluginUtils.h"
#include "src/plugins/CatapultUpgradePlugin.h"
#include "plugins/txes/upgrade/src/model/CatapultUpgradeEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

	namespace {
		struct CatapultUpgradePluginTraits {
		public:
			template<typename TAction>
			static void RunTestAfterRegistration(TAction action) {
				// Arrange:
				auto config = model::BlockChainConfiguration::Uninitialized();
				config.Plugins.emplace(PLUGIN_NAME(config), utils::ConfigurationBag({{
					"",
					{
						{ "minUpgradePeriod", "360" },
					}
				}}));

				auto manager = test::CreatePluginManager(config);
				RegisterCatapultUpgradeSubsystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return { model::Entity_Type_Catapult_Upgrade };
			}

			static std::vector<std::string> GetCacheNames() {
				return { "CatapultUpgradeCache" };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return { ionet::PacketType::Catapult_Upgrade_Infos };
			}

			static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
				return { ionet::PacketType::Catapult_Upgrade_State_Path };
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return { "UPGRADE C" };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return {
					"CatapultUpgradePluginConfigValidator",
				};
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return {
					"CatapultUpgradeSignerValidator",
					"CatapultUpgradeValidator",
					"CatapultVersionValidator",
				};
			}

			static std::vector<std::string> GetObserverNames() {
				return {
					"CatapultUpgradeObserver",
				};
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return GetObserverNames();
			}
		};
	}

	DEFINE_PLUGIN_TESTS(CatapultUpgradePluginTests, CatapultUpgradePluginTraits)
}}
