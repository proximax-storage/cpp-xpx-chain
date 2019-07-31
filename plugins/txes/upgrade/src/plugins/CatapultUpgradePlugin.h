/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/plugins.h"

namespace catapult { namespace plugins { class PluginManager; } }

namespace catapult { namespace plugins {

	/// Registers catapult upgrade support with \a manager.
	PLUGIN_API
	void RegisterCatapultUpgradeSubsystem(PluginManager& manager);
}}
