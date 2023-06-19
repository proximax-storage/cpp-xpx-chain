/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MockDbrbViewFetcher.h"
//#include "src/cache/DbrbViewCache.h"

namespace catapult { namespace mocks {

	dbrb::ViewData MockDbrbViewFetcher::getView(Timestamp timestamp) const {
		dbrb::ViewData view;
//		for (auto iter = m_expirationTimes.rbegin(); iter != m_expirationTimes.rend(); ++iter) {
//			if (iter->first <= timestamp)
//				break;
//
//			view.insert(iter->second.begin(), iter->second.end());
//		}

		return view;
	}

	Timestamp MockDbrbViewFetcher::getExpirationTime(const dbrb::ProcessId& processId) const {
//		auto iter = m_processes.find(processId);
//		if (iter != m_processes.end())
//			return iter->second;

		return Timestamp();
	}
}}
