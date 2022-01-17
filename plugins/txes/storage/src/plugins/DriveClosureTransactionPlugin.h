/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/plugins.h"
#include <memory>

namespace catapult {
	namespace model { class TransactionPlugin; }
	namespace config { class ImmutableConfiguration; }
}

namespace catapult { namespace plugins {

	/// Creates a drive closure transaction plugin.
	PLUGIN_API
	std::unique_ptr<model::TransactionPlugin> CreateDriveClosureTransactionPlugin(const config::ImmutableConfiguration& config);
}}
