/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MockCommitteeManager.h"

namespace catapult { namespace mocks {
	const chain::Committee& MockCommitteeManager::selectCommittee(const model::NetworkConfiguration& config) {
		return chain::Committee(0);
	}
	void MockCommitteeManager::reset() {}
	double MockCommitteeManager::weight(const Key& accountKey) const {
		return Weight;
	}
	void MockCommitteeManager::setCommittee(const chain::Committee& committee) {
		m_committee = committee;
	}
}}
