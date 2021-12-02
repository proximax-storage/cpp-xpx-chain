/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "drive/Replicator.h"
#include "TransactionSender.h"
#include "catapult/state/StorageState.h"
#include "catapult/extensions/ServiceState.h"

namespace catapult { namespace storage {

	std::unique_ptr<sirius::drive::ReplicatorEventHandler> CreateReplicatorEventHandler(
		TransactionSender&& transactionSender,
		state::StorageState& storageState,
		const extensions::ServiceState& serviceState);
}}
