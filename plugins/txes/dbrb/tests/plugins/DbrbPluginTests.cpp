/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/DbrbPlugin.h"
#include "plugins/txes/dbrb/src/model/DbrbEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"

namespace catapult { namespace plugins {

	namespace {
		struct DbrbPluginTraits {
		public:
			template<typename TAction>
			static void RunTestAfterRegistration(TAction action) {
				// Arrange:
				auto config = model::NetworkConfiguration::Uninitialized();
				config.Plugins.emplace(PLUGIN_NAME(dbrb), utils::ConfigurationBag({{
																	   "",
																	   {
																			   { "enabled", "true" },
																	   }
															   }}));

				auto manager = test::CreatePluginManager(config);
				RegisterDbrbSubsystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return {
					model::Entity_Type_AddDbrbProcess
				};
			}

			static std::vector<std::string> GetCacheNames() {
				return { "DbrbViewCache" };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return { ionet::PacketType::DbrbView_Infos };
			}

			static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
				return { ionet::PacketType::DbrbView_State_Path };
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return { "DBRB VIEW C" };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return {};
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return { "AddDbrbProcessValidator" };
			}

			static std::vector<std::string> GetObserverNames() {
				return {
					"DbrbProcessPruningObserver", "AddDbrbProcessObserver"
				};
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return GetObserverNames();
			}
		};
	}

	DEFINE_PLUGIN_TESTS(DbrbPluginTests, DbrbPluginTraits)
}}
