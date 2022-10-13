/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/PluginUtils.h"
#include "src/plugins/ContractPlugin.h"
#include "plugins/txes/contract_v2/src/model/ContractEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

    namespace {
		struct ContractV2PluginTraits {
            public:
                template<typename TAction>
                static void RunTestAfterRegistration(TAction action) {
                    // Arrange:
                    auto config = model::NetworkConfiguration::Uninitialized();
                    auto manager = test::CreatePluginManager(config);
                    RegisterContractSubsystem(manager);

                    // Act:
                    action(manager);
                }

            public:
                static std::vector<model::EntityType> GetTransactionTypes() {
                    return { model::Entity_Type_Deploy };
                }

                static std::vector<std::string> GetCacheNames() {
                    return { };
                }

                static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
                    return { ionet::PacketType::Contract_v2_Infos };
                }

                static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
                    return { ionet::PacketType::Contract_v2_State_Path };
                }

                static std::vector<std::string> GetDiagnosticCounterNames() {
                    return {  };
                }

                static std::vector<std::string> GetStatelessValidatorNames() {
                    return {
                    };
				};

                static std::vector<std::string> GetObserverNames() {
                    return {
                    };
                };
        };
    }

    DEFINE_PLUGIN_TESTS(ContractV2PluginTests, ContractV2PluginTraits)
}}