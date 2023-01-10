/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include <executor/ExecutorEventHandler.h>

namespace catapult::contract {

class DefaultExecutorEventHandler: public sirius::contract::ExecutorEventHandler {
	void endBatchTransactionIsReady(const sirius::contract::EndBatchExecutionTransactionInfo& info) override;
	void endBatchSingleTransactionIsReady(
			const sirius::contract::EndBatchExecutionSingleTransactionInfo& info) override;

public:
	void synchronizationSingleTransactionIsReady(
			const sirius::contract::SynchronizationSingleTransactionInfo& info) override;
};

}