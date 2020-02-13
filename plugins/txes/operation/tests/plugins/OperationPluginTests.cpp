/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/PluginUtils.h"
#include "src/plugins/OperationPlugin.h"
#include "src/model/OperationEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

	namespace {
		struct OperationPluginTraits {
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
						{ "maxOperationDuration", "30d" },
					}
				}}));

				auto manager = test::CreatePluginManager(config);
				RegisterOperationSubsystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return {
					model::Entity_Type_OperationIdentify,
					model::Entity_Type_StartOperation,
					model::Entity_Type_EndOperation,
				};
			}

			static std::vector<std::string> GetCacheNames() {
				return { "OperationCache" };
			}

			static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
				return { ionet::PacketType::Operation_State_Path };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return { ionet::PacketType::Operation_Infos };
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return { "OPERATION C" };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return {
					"OperationPluginConfigValidator",
					"StartOperationValidator",
					"AggregateTransactionValidator",
					"OperationMosaicValidator",
				};
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return {
					"OperationDurationValidator",
					"OperationIdentifyValidator",
					"EndOperationValidator",
				};
			}

			static std::vector<std::string> GetObserverNames() {
				return {
					"StartOperationObserver",
					"ExpiredOperationObserver",
					"EndOperationObserver",
					"AggregateTransactionHashObserver",
					"OperationTouchObserver",
				};
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return GetObserverNames();
			}
		};
	}

	DEFINE_PLUGIN_TESTS(OperationPluginTests, OperationPluginTraits)
}}
