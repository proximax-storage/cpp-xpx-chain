#include "Validators.h"
#include "src/config/HelloConfiguration.h"

namespace catapult { namespace validators {

        using Notification = model::HelloMessageCountNotification<1>;

        DECLARE_STATEFUL_VALIDATOR(HelloMessageCount, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
            return MAKE_STATEFUL_VALIDATOR(HelloMessageCount, [pConfigHolder](const auto& notification, const auto& context) {
                // do checking here
                const model::NetworkConfiguration& networkConfig = pConfigHolder->Config(context.Height).Network;;
                const auto& pluginConfig = networkConfig.GetPluginConfiguration<config::HelloConfiguration>(PLUGIN_NAME_HASH(hello));
                return (notification.MessageCount > pluginConfig.messageCount || notification.MessageCount <= 0)? Failure_Hello_MessageCount_Invalid : ValidationResult::Success;
                
            });
        }
    }}
