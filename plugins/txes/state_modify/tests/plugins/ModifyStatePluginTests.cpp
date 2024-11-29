/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/StateModifyPlugin.h"
#include "src/model/ModifyStateEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"

namespace catapult { namespace plugins {

	namespace {
		struct ModifyStatePluginTraits {
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
				RegisterModifyStateSubsystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return {
					model::Entity_Type_ModifyState
				};
			}

			static std::vector<std::string> GetCacheNames() {
				return {  };
			}

			static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
				return { };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return {  };
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return {};
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return {
					"ModifyStatePluginConfigValidator",
				};
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return {
					"ModifyStateValidator",
				};
			}

			static std::vector<std::string> GetObserverNames() {
				return {
					"ModifyStateObserver",
				};
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return GetObserverNames();
			}
		};
	}

	DEFINE_PLUGIN_TESTS(ModifyStatePluginTests, ModifyStatePluginTraits)
}}
