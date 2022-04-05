/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/plugins.h"
#include <memory>

namespace catapult { namespace model { class TransactionPlugin; } }

namespace catapult { namespace plugins {

	/// Creates a place and exchange SDA-SDA offer transaction plugin.
	PLUGIN_API
	std::unique_ptr<model::TransactionPlugin> CreatePlaceSdaExchangeOfferTransactionPlugin();
}}
