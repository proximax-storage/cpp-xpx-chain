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

	Key MockCommitteeManager::getBootKey(const Key& harvestKey, const model::NetworkConfiguration& config) const {
		return Key();
	}

	void MockCommitteeManager::reset() {}

	chain::HarvesterWeight MockCommitteeManager::weight(const Key& accountKey, const model::NetworkConfiguration& config) const {
		return Weight;
	}
	
	chain::HarvesterWeight MockCommitteeManager::zeroWeight() const {
		return chain::HarvesterWeight{ .n = 0 };
	}

	void MockCommitteeManager::add(chain::HarvesterWeight& weight, const chain::HarvesterWeight& delta) const {
		weight.d += delta.d;
	}

	void MockCommitteeManager::mul(chain::HarvesterWeight& weight, double multiplier) const {
		weight.d *= multiplier;
	}

	bool MockCommitteeManager::ge(const chain::HarvesterWeight& weight1, const chain::HarvesterWeight& weight2) const {
		return false;
	}

	bool MockCommitteeManager::eq(const chain::HarvesterWeight& weight1, const chain::HarvesterWeight& weight2) const {
		return weight1.d == weight1.d;
	}

	std::string MockCommitteeManager::str(const chain::HarvesterWeight& weight) const {
		return "";
	}
	
	void MockCommitteeManager::setCommittee(const chain::Committee& committee) {
		m_committee = committee;
	}
}}
