/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/validators/ValidatorUtils.h"

namespace catapult { namespace validators {

	using Notification = model::StartOperationNotification<1>;

	DEFINE_STATELESS_VALIDATOR(StartOperation, [](const auto& notification) {
		std::set<Key> executors;
		auto pExecutor = notification.ExecutorsPtr;
		for (auto i = 0u; i < notification.ExecutorCount; ++i, ++pExecutor) {
			if (!executors.insert(*pExecutor).second)
				return Failure_Operation_Executor_Redundant;
		}

		return ValidationResult::Success;
	})
}}
