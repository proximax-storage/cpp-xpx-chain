/**
*** FOR TRAINING PURPOSES ONLY
**/

#include "catapult/plugins/PluginUtils.h"
#include "src/plugins/HelloPlugin.h"
#include "plugins/txes/hello/src/model/HelloEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

        namespace {
            struct HelloPluginTraits : public test::EmptyPluginTraits {
            public:
                template<typename TAction>
                static void RunTestAfterRegistration(TAction action) {
                    // Arrange:
                    auto config = model::NetworkConfiguration::Uninitialized();
                    config.Plugins.emplace(PLUGIN_NAME(transfer), utils::ConfigurationBag({{ "", { { "messageCount", "10" } } }}));
                    auto manager = test::CreatePluginManager(config);
                    RegisterHelloSubsystem(manager);

                    // Act:
                    action(manager);
                }

            public:
                static std::vector<model::EntityType> GetTransactionTypes() {
                    return { model::Entity_Type_Hello };
                }

                static std::vector<std::string> GetStatelessValidatorNames() {
                    return { "HelloPluginConfigValidator" };
                }

                static std::vector<std::string> GetStatefulValidatorNames() {
                    return { "HelloMessageCountValidator" };
                }

                static std::vector<std::string> GetObserverNames() {
                    return {
                            "HelloObserver",
                    };
                }

                static std::vector<std::string> GetPermanentObserverNames() {
                    return GetObserverNames();
                }

                static std::vector<std::string> GetCacheNames() {
                    return { "HelloCache" };
                }

                static std::vector<std::string> GetDiagnosticCounterNames() {
                    return { "HELLO C" };
                }

                // PacketTypes are defined in src\catapult\ionet\PacketType.h
                static std::vector<ionet::PacketType> GetDiagnosticPacketTypes() {
                    return { ionet::PacketType::Hello_Infos };
                }

                static std::vector<ionet::PacketType> GetNonDiagnosticPacketTypes() {
                    return { ionet::PacketType::Hello_State_Path };
                }

            };
        }

        // First parameter is test group name or TEST_CLASS
        // second parameter is traits struct defined above
        // macro defined in
        DEFINE_PLUGIN_TESTS(HelloPluginTests, HelloPluginTraits)
    }}
