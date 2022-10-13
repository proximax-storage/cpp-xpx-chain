/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ContractPlugin.h"
#include "src/plugins/DeployTransactionPlugin.h"
#include "catapult/plugins/PluginManager.h"

namespace catapult { namespace plugins {

    void RegisterContractSubsystem(PluginManager& manager) {
		    manager.addTransactionSupport(CreateDeployTransactionPlugin());
    }
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterContractSubsystem(manager);
}