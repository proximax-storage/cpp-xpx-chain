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
			Weight.n = 0;
		}

	public:
		const chain::Committee& selectCommittee(const model::NetworkConfiguration& config) override;
		Key getBootKey(const Key& harvestKey, const model::NetworkConfiguration& config) const override;
		void reset() override;
		chain::Committee committee() const override {
			return m_committee;
		}
		chain::HarvesterWeight weight(const Key& accountKey, const model::NetworkConfiguration& config) const override;
		chain::HarvesterWeight zeroWeight() const override;
		void add(chain::HarvesterWeight& weight, const chain::HarvesterWeight& delta) const override;
		void mul(chain::HarvesterWeight& weight, double multiplier) const override;
		bool ge(const chain::HarvesterWeight& weight1, const chain::HarvesterWeight& weight2) const override;
		bool eq(const chain::HarvesterWeight& weight1, const chain::HarvesterWeight& weight2) const override;
		std::string str(const chain::HarvesterWeight& weight) const override;
		void setCommittee(const chain::Committee& committee);
		void logCommittee() const override {}

		chain::HarvesterWeight Weight{};
	};
}}
