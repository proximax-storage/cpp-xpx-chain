/**
*** Copyright (c) 2018-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "catapult/plugins/PluginUtils.h"
#include "src/plugins/ContractPlugin.h"
#include "plugins/txes/contract/src/model/ContractEntityType.h"
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
						{ "modifyContractTransactionSupportedVersions", "3" },
						{ "minPercentageOfApproval", "100" },
						{ "minPercentageOfRemoval", "66" },
					}
				}}));

				auto pConfigHolder = std::make_shared<config::LocalNodeConfigurationHolder>();
				pConfigHolder->SetBlockChainConfig(config);
				PluginManager manager(pConfigHolder, StorageConfiguration());
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
