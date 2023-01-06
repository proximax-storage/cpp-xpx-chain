/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DefaultExecutorEventHandler.h"

namespace catapult::contract {

void DefaultExecutorEventHandler::endBatchTransactionIsReady(
		const sirius::contract::EndBatchExecutionTransactionInfo& info) {}
void DefaultExecutorEventHandler::endBatchSingleTransactionIsReady(
		const sirius::contract::EndBatchExecutionSingleTransactionInfo& info) {}

}