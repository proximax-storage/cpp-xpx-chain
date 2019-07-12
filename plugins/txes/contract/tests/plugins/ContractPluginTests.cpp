/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/PluginUtils.h"
#include "src/plugins/ContractPlugin.h"
#include "plugins/txes/contract/src/model/ContractEntityType.h"
#include "tests/test/core/mocks/MockLocalNodeConfigurationHolder.h"
#include "tests/test/plugins/PluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

	namespace {
		struct ContractPluginTraits {
		public:
			template<typename TAction>
			static void RunTestAfterRegistration(TAction action) {
				// Arrange:
				auto config = model::BlockChainConfiguration::Uninitialized();
				config.Plugins.emplace(PLUGIN_NAME(contract), utils::ConfigurationBag({{
					"",
					{
						{ "minPercentageOfApproval", "100" },
						{ "minPercentageOfRemoval", "66" },
					}
				}}));

				auto pConfigHolder = std::make_shared<config::MockLocalNodeConfigurationHolder>();
				pConfigHolder->SetBlockChainConfig(config);
				PluginManager manager(pConfigHolder, StorageConfiguration(), config::InflationConfiguration::Uninitialized());
				RegisterContractSubsystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return { model::Entity_Type_Modify_Contract };
			}

			static std::vector<std::string> GetCacheNames() {
				return { "ReputationCache", "ContractCache" };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return {
					ionet::PacketType::Contract_Infos,
					ionet::PacketType::Reputation_Infos,
				};
			}

			static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
				return { ionet::PacketType::Contract_State_Path, ionet::PacketType::Reputation_State_Path };
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return { "CONTRACT C", "REPUTATION C" };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return {
					"ModifyContractCustomersValidator",
					"ModifyContractExecutorsValidator",
					"ModifyContractVerifiersValidator",
					"PluginConfigValidator",
				};
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return {
					"ModifyContractInvalidCustomersValidator",
					"ModifyContractInvalidExecutorsValidator",
					"ModifyContractInvalidVerifiersValidator",
					"ModifyContractDurationValidator",
				};
			}

			static std::vector<std::string> GetObserverNames() {
				return {
					"ModifyContractObserver",
					"ReputationUpdateObserver"
				};
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return GetObserverNames();
			}
		};
	}

	DEFINE_PLUGIN_TESTS(ContractPluginTests, ContractPluginTraits)
}}
