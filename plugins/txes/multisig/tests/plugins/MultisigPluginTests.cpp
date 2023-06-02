/**
*** Copyright (c) 2016-present,
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
#include "src/plugins/MultisigPlugin.h"
#include "plugins/txes/multisig/src/model/MultisigEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

	namespace {
		struct MultisigPluginTraits {
		public:
			template<typename TAction>
			static void RunTestAfterRegistration(TAction action) {
				// Arrange:
				auto config = model::NetworkConfiguration::Uninitialized();
				config.Plugins.emplace(PLUGIN_NAME(multisig), utils::ConfigurationBag({{
					"",
					{
						{ "maxMultisigDepth", "0" },
						{ "maxCosignersPerAccount", "0" },
						{ "maxCosignedAccountsPerAccount", "0" }
					}
				}}));

				auto manager = test::CreatePluginManager(config);
				RegisterMultisigSubsystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return { model::Entity_Type_Modify_Multisig_Account };
			}

			static std::vector<std::string> GetCacheNames() {
				return { "MultisigCache" };
			}

			static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
				return { ionet::PacketType::Multisig_State_Path };
			}

			static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
				return { ionet::PacketType::Multisig_Infos };
			}

			static std::vector<std::string> GetDiagnosticCounterNames() {
				return { "MULTISIG C" };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return { "MultisigPluginConfigValidator", "ModifyMultisigCosignersValidator" };
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return {
					"MultisigAggregateEligibleCosignersV1Validator",
					"MultisigAggregateSufficientCosignersV1Validator",
					"MultisigAggregateSufficientCosignersV2Validator",
					"ModifyMultisigMaxCosignedAccountsValidator",
					"ModifyMultisigLoopAndLevelValidator",
					"MultisigAggregateEligibleCosignersV2Validator",
					"MultisigPermittedOperationValidator",
					"ModifyMultisigMaxCosignersValidator",
					"ModifyMultisigInvalidCosignersValidator",
					"ModifyMultisigInvalidSettingsValidator"
				};
			}

			static std::vector<std::string> GetObserverNames() {
				return { "ModifyMultisigCosignersObserver", "ModifyMultisigSettingsObserver", "UpgradeMultisigObserver" };
			}

			static std::vector<std::string> GetPermanentObserverNames() {
				return { "ModifyMultisigCosignersObserver", "ModifyMultisigSettingsObserver", "UpgradeMultisigObserver" };
			}
		};
	}

	DEFINE_PLUGIN_TESTS(MultisigPluginTests, MultisigPluginTraits)
}}
