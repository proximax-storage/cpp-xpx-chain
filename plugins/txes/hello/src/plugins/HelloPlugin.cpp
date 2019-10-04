/**
*** FOR TRAINING PURPOSES ONLY
**/

#include "HelloPlugin.h"
#include "HelloTransactionPlugin.h"
#include "src/config/HelloConfiguration.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/PluginManager.h"

namespace catapult { namespace plugins {

        void RegisterHelloSubsystem(PluginManager& manager) {

            //-- created via call to DEFINE_TRANSACTION_PLUGIN_FACTORY in HelloTransactionPlugin.cpp
            manager.addTransactionSupport(CreateHelloTransactionPlugin());

            manager.addStatelessValidatorHook([](auto& builder) {
                builder
                        .add(validators::CreateHelloPluginConfigValidator());
            });

            const auto& pConfigHolder = manager.configHolder();
            manager.addStatefulValidatorHook([pConfigHolder](auto& builder) {
                builder
                        .add(validators::CreateHelloMessageCountValidator(pConfigHolder)); // created in Validators.h using macro DECLARE_STATEFUL_VALIDATOR
            });

        }
    }}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterHelloSubsystem(manager);
}
