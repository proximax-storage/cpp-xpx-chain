/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/NetworkConfigPlugin.h"
#include "plugins/txes/config/src/model/NetworkConfigEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"

namespace catapult { namespace plugins {

	namespace {
		struct NetworkConfigPluginTraits {
		public:
			template<typename TAction>
			static void RunTestAfterRegistration(TAction action) {
				// Arrange:
				auto config = model::NetworkConfiguration::Uninitialized();
				config.Plugins.emplace(PLUGIN_NAME(config), utils::ConfigurationBag({{
					"",
					{
						{ "maxBlockChainConfigSize", "1MB" },
						{ "maxSupportedEntityVersionsSize", "1MB" },
					}
				}}));

				auto manager = test::CreatePluginManager(config);
				RegisterNetworkConfigSubsystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return { model::Entity_Type_Network_Config };
			}

			static std::vector<std::string> GetCacheNames() {
				return { "NetworkConfigCache" };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return { ionet::PacketType::Network_Config_Infos };
			}

			static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
				return { ionet::PacketType::Network_Config_State_Path };
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return { "CONFIG C" };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return {
					"NetworkConfigPluginConfigValidator",
				};
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return {
					"NetworkConfigSignerValidator",
					"PluginAvailableValidator",
					"NetworkConfigValidator",
				};
			}

			static std::vector<std::string> GetObserverNames() {
				return {
					"NetworkConfigObserver",
				};
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return GetObserverNames();
			}
		};
	}

	DEFINE_PLUGIN_TESTS(NetworkConfigPluginTests, NetworkConfigPluginTraits)
}}
