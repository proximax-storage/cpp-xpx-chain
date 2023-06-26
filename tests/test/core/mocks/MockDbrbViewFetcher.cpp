/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MockDbrbViewFetcher.h"

namespace catapult { namespace mocks {

	dbrb::ViewData MockDbrbViewFetcher::getView(Timestamp timestamp) const {
		return {};
	}

	Timestamp MockDbrbViewFetcher::getExpirationTime(const dbrb::ProcessId& processId) const {
		return Timestamp();
	}
}}
