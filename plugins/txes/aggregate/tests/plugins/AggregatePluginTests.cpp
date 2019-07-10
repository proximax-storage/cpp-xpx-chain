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
#include "src/plugins/AggregatePlugin.h"
#include "src/model/AggregateEntityType.h"
#include "tests/test/core/mocks/MockLocalNodeConfigurationHolder.h"
#include "tests/test/plugins/PluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

	namespace {
		struct AggregatePluginTraits : public test::EmptyPluginTraits {
		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return { model::Entity_Type_Aggregate_Complete, model::Entity_Type_Aggregate_Bonded };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return { "PluginConfigValidator" };
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return { "BasicAggregateCosignaturesValidator", "StrictAggregateCosignaturesValidator", "AggregateTransactionTypeValidator" };
			}

			template<typename TAction>
			static void RunTestAfterRegistration(TAction action) {
				// Arrange:
				auto config = model::BlockChainConfiguration::Uninitialized();
				config.Plugins.emplace(PLUGIN_NAME(aggregate), utils::ConfigurationBag({{
					"",
					{
						{ "maxTransactionsPerAggregate", "0" },
						{ "maxCosignaturesPerAggregate", "0" },
						{ "enableStrictCosignatureCheck", "false" },
						{ "enableBondedAggregateSupport", "true" },
					}
				}}));

				auto pConfigHolder = std::make_shared<config::MockLocalNodeConfigurationHolder>();
				pConfigHolder->SetBlockChainConfig(config);
				PluginManager manager(pConfigHolder, StorageConfiguration());
				RegisterAggregateSubsystem(manager);

				// Act:
				action(manager);
			}
		};
	}

	DEFINE_PLUGIN_TESTS(AggregatePluginTests, AggregatePluginTraits)
}}
