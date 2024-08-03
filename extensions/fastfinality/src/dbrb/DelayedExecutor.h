/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/functions.h"
#include <cstdint>
#include <memory>

namespace catapult { namespace thread { class IoThreadPool; }}

namespace catapult { namespace dbrb {

	class DelayedExecutor {
	public:
		virtual ~DelayedExecutor() = default;

	public:
		virtual void execute(uint64_t delayMillis, const action& callback) = 0;
		virtual void cancel() = 0;
	};

	std::shared_ptr<DelayedExecutor> CreateDelayedExecutor(const std::shared_ptr<thread::IoThreadPool>& pPool);
}}