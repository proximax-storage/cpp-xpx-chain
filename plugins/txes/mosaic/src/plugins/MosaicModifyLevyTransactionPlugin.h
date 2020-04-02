#pragma once

#pragma once
#include "catapult/plugins.h"
#include <memory>

namespace catapult { namespace model { class TransactionPlugin; } }

namespace catapult { namespace plugins {

	/// Creates a mosaic supply change transaction plugin.
	PLUGIN_API
	std::unique_ptr<model::TransactionPlugin> CreateMosaicModifyLevyTransactionPlugin();
}}
