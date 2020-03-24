/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/PluginUtils.h"
#include "src/plugins/BlockchainUpgradePlugin.h"
#include "plugins/txes/upgrade/src/model/BlockchainUpgradeEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

	namespace {
		struct BlockchainUpgradePluginTraits {
		public:
			template<typename TAction>
			static void RunTestAfterRegistration(TAction action) {
				// Arrange:
				auto config = model::NetworkConfiguration::Uninitialized();
				config.Plugins.emplace(PLUGIN_NAME(config), utils::ConfigurationBag({{
					"",
					{
						{ "minUpgradePeriod", "360" },
					}
				}}));

				auto manager = test::CreatePluginManager(config);
				RegisterBlockchainUpgradeSubsystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return { model::Entity_Type_Blockchain_Upgrade };
			}

			static std::vector<std::string> GetCacheNames() {
				return { "BlockchainUpgradeCache" };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return { ionet::PacketType::Blockchain_Upgrade_Infos };
			}

			static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
				return { ionet::PacketType::Blockchain_Upgrade_State_Path };
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return { "UPGRADE C" };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return {
					"BlockchainUpgradePluginConfigValidator",
				};
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return {
					"BlockchainUpgradeSignerValidator",
					"BlockchainVersionValidator",
					"BlockchainUpgradeValidator",
				};
			}

			static std::vector<std::string> GetObserverNames() {
				return {
					"BlockchainUpgradeObserver",
				};
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return GetObserverNames();
			}
		};
	}

	DEFINE_PLUGIN_TESTS(BlockchainUpgradePluginTests, BlockchainUpgradePluginTraits)
}}
