/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/extensions/ServiceRegistrar.h"

namespace catapult { namespace dbrb { class TransactionSender; }}

namespace catapult { namespace fastfinality {

	/// Creates a registrar for a weighted voting shutdown service.
	DECLARE_SERVICE_REGISTRAR(FastFinalityShutdown)(std::shared_ptr<dbrb::TransactionSender> pTransactionSender);
}}
