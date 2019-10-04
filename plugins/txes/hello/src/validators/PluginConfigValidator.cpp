/**
*** FOR TRAINING PURPOSES ONLY
**/

#include "Validators.h"
#include "src/config/HelloConfiguration.h"

namespace catapult { namespace validators {

        // defined in catapult\plugins\PluginUtils.h
        //first param is suffix use for PLUGIN_NAME, ex: catapult.plugins.hello
        // second parameter is CONFIG_NAME
        // 3rd parameter is version
        DEFINE_PLUGIN_CONFIG_VALIDATOR(hello, Hello, 1)
    }}
