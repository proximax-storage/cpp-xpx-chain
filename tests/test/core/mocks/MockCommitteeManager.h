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
		MockCommitteeManager() {
			m_committee = chain::Committee(0);
			m_committee.BlockProposer = test::GenerateRandomByteArray<Key>();
		}

	public:
		const chain::Committee& selectCommittee(const model::NetworkConfiguration& config) override;
		void reset() override;
		double weight(const Key& accountKey) const override;
		void setCommittee(const chain::Committee& committee);

		double Weight = 0;
	};
}}
