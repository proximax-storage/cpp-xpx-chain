/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/PluginUtils.h"
#include "src/plugins/CatapultConfigPlugin.h"
#include "plugins/txes/config/src/model/CatapultConfigEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

	namespace {
		struct CatapultConfigPluginTraits {
		public:
			template<typename TAction>
			static void RunTestAfterRegistration(TAction action) {
				// Arrange:
				auto config = model::BlockChainConfiguration::Uninitialized();
				config.Plugins.emplace(PLUGIN_NAME(config), utils::ConfigurationBag({{
					"",
					{
						{ "maxBlockChainConfigSize", "1MB" },
						{ "maxSupportedEntityVersionsSize", "1MB" },
					}
				}}));

				auto manager = test::CreatePluginManager(config);
				RegisterCatapultConfigSubsystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return { model::Entity_Type_Catapult_Config };
			}

			static std::vector<std::string> GetCacheNames() {
				return { "CatapultConfigCache" };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return { ionet::PacketType::Catapult_Config_Infos };
			}

			static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
				return { ionet::PacketType::Catapult_Config_State_Path };
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return { "CONFIG C" };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return {
					"CatapultConfigPluginConfigValidator",
				};
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return {
					"CatapultConfigSignerValidator",
					"CatapultConfigValidator",
				};
			}

			static std::vector<std::string> GetObserverNames() {
				return {
					"CatapultConfigObserver",
				};
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return GetObserverNames();
			}
		};
	}

	DEFINE_PLUGIN_TESTS(CatapultConfigPluginTests, CatapultConfigPluginTraits)
}}
