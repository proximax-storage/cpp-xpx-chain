/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/catapult/dbrb/DbrbViewFetcher.h"
#include "catapult/cache/CatapultCache.h"
#include "catapult/dbrb/DbrbViewFetcher.h"
#include "catapult/utils/Hashers.h"
#include <map>

namespace catapult { namespace mocks {
	class MockDbrbViewFetcher : public dbrb::DbrbViewFetcher {
	public:
		explicit MockDbrbViewFetcher() = default;

	public:
		dbrb::ViewData getView(Timestamp timestamp) const override {}
		Timestamp getExpirationTime(const dbrb::ProcessId& processId) const override {}
	};
}}
