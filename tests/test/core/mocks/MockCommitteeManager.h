/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/catapult/chain/CommitteeManager.h"
#include "tests/test/nodeps/Random.h"

namespace catapult { namespace mocks {
	class MockCommitteeManager : public chain::CommitteeManager {
	public:
		MockCommitteeManager() : MockCommitteeManager(test::GenerateRandomByteArray<Key>()) {}

		MockCommitteeManager(const Key& accountKey) {
			m_committee = chain::Committee(0);
			m_committee.BlockProposer = accountKey;
		}

	public:
		const chain::Committee& selectCommittee(const model::NetworkConfiguration& config) override;
		void reset() override;
		double weight(const Key& accountKey) const override;
		void setCommittee(const chain::Committee& committee);

		double Weight = 0;
	};
}}
