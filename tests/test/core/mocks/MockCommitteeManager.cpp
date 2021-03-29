//
// Created by newman on 05.03.2021.
//

#include "MockCommitteeManager.h"

namespace catapult { namespace mocks {
	const chain::Committee& MockCommitteeManager::selectCommittee(const model::NetworkConfiguration& config) {
		return catapult::chain::Committee(0);
	}
	void MockCommitteeManager::reset() {}
	double MockCommitteeManager::weight(const Key& accountKey) const {
		return Weight;
	}
	void MockCommitteeManager::setCommittee(const chain::Committee& committee) {
		m_committee = committee;
	}
}} // namespace catapult::mocks
