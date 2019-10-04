/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once

#include "catapult/plugins.h"

namespace catapult { namespace plugins { class PluginManager; } }

namespace catapult { namespace plugins {

        /// Registers transfer support with \a manager.
        PLUGIN_API
        void RegisterHelloSubsystem(PluginManager& manager);
    }}
