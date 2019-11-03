/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/PluginUtils.h"
#include "src/plugins/ExchangePlugin.h"
#include "plugins/txes/exchange/src/model/ExchangeEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

	namespace {
		struct ExchangePluginTraits {
		public:
			template<typename TAction>
			static void RunTestAfterRegistration(TAction action) {
				// Arrange:
				auto config = model::NetworkConfiguration::Uninitialized();
				config.Plugins.emplace(PLUGIN_NAME(exchange), utils::ConfigurationBag({{
					"",
					{
						{ "enabled", "true" },
						{ "maxOfferDuration", "1000" },
					}
				}}));

				auto manager = test::CreatePluginManager(config);
				RegisterExchangeSubsystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return {
					model::Entity_Type_Exchange_Offer,
					model::Entity_Type_Exchange,
					model::Entity_Type_Remove_Exchange_Offer,
				};
			}

			static std::vector<std::string> GetCacheNames() {
				return { "ExchangeCache" };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return { ionet::PacketType::Exchange_Infos };
			}

			static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
				return { ionet::PacketType::Exchange_State_Path };
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return { "EXCHANGE C" };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return {
					"ExchangePluginConfigValidator",
				};
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return {
					"OfferValidator",
					"ExchangeValidator",
					"RemoveOfferValidator",
				};
			}

			static std::vector<std::string> GetObserverNames() {
				return {
					"OfferObserver",
					"ExchangeObserver",
					"RemoveOfferObserver",
					"CleanupOffersObserver",
				};
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return GetObserverNames();
			}
		};
	}

	DEFINE_PLUGIN_TESTS(ExchangePluginTests, ExchangePluginTraits)
}}
