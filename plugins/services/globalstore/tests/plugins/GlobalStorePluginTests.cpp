/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "src/plugins/GlobalStoreCacheSystem.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

	namespace {
		struct GlobalStorePluginTraits {
		public:
			template<typename TAction>
			static void RunTestAfterRegistration(TAction action) {
				// Arrange:
				test::MutableBlockchainConfiguration config;
				config.Network.Plugins.emplace("catapult.plugins.globalstore", utils::ConfigurationBag({{
					"",
					{
						{"enabled", "false"},
					}
				}}));

				auto manager = test::CreatePluginManager(config.ToConst().Network);
				RegisterGlobalStoreCacheSystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return {
				};
			}

			static std::vector<std::string> GetCacheNames() {
				return { "GlobalStoreCache" };
			}

			static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
				return {  };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return {  };
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return { };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return {

				};
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return
				{

				};
			}

			static std::vector<std::string> GetObserverNames() {
				return {

				};
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return GetObserverNames();
			}
		};
	}

	DEFINE_PLUGIN_TESTS(GlobalStorePluginTests, GlobalStorePluginTraits)
}}
