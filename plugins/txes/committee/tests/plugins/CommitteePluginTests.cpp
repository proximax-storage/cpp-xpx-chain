/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/CommitteePlugin.h"
#include "plugins/txes/committee/src/model/CommitteeEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"

namespace catapult { namespace plugins {

	namespace {
		struct CommitteePluginTraits {
		public:
			template<typename TAction>
			static void RunTestAfterRegistration(TAction action) {
				// Arrange:
				auto config = model::NetworkConfiguration::Uninitialized();
				config.Plugins.emplace(PLUGIN_NAME(committee), utils::ConfigurationBag({{
					"",
					{
						{ "enabled", "true" },
					}
				}}));

				auto manager = test::CreatePluginManager(config);
				RegisterCommitteeSubsystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return {
					model::Entity_Type_AddHarvester,
					model::Entity_Type_RemoveHarvester,
				};
			}

			static std::vector<std::string> GetCacheNames() {
				return { "CommitteeCache" };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return {};
			}

			static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
				return {};
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return { "COMMITTEE C" };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return {
					"CommitteePluginConfigValidator",
				};
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return {
					"CommitteeValidator",
					"AddHarvesterValidator",
					"RemoveHarvesterValidator",
				};
			}

			static std::vector<std::string> GetObserverNames() {
				return {
					"UpdateHarvestersV1Observer",
					"ActiveHarvestersV1Observer",
					"UpdateHarvestersV2Observer",
					"ActiveHarvestersV2Observer",
					"InactiveHarvestersObserver",
					"RemoveDbrbProcessByNetworkObserver",
					"UpdateHarvestersV3Observer",
					"ActiveHarvestersV3Observer",
					"UpdateHarvestersV4Observer",
					"ActiveHarvestersV4Observer",
					"AddHarvesterObserver",
					"RemoveHarvesterObserver",
				};
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return GetObserverNames();
			}
		};
	}

	DEFINE_PLUGIN_TESTS(CommitteePluginTests, CommitteePluginTraits)
}}
