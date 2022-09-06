/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/LiquidityProviderPlugin.h"
#include "src/model/LiquidityProviderEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"

namespace catapult { namespace plugins {

	namespace {
		struct LiquidityProviderPluginTraits {
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
				RegisterLiquidityProviderSubsystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return {
					model::Entity_Type_CreateLiquidityProvider,
					model::Entity_Type_ManualRateChange,
				};
			}

			static std::vector<std::string> GetCacheNames() {
				return { "LiquidityProviderCache" };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return { ionet::PacketType::LiquidityProvider_Infos };
			}

			static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
				return { ionet::PacketType::LiquidityProvider_State_Path };
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return { "LP C" };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return {
					"LiquidityProviderPluginConfigValidator"
				};
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return {
					"CreateLiquidityProviderValidator",
					"ManualRateChangeValidator",
					"DebitMosaicValidator",
					"CreditMosaicValidator"
				};
			}

			static std::vector<std::string> GetObserverNames() {
				return {
					"SlashingObserver",
					"CreateLiquidityProviderObserver",
					"ManualRateChangeObserver",
					"DebitMosaicObserver",
					"CreditMosaicObserver"
				};
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return GetObserverNames();
			}
		};
	}

	 DEFINE_PLUGIN_TESTS(LiquidityProviderTests, LiquidityProviderPluginTraits)
}}
