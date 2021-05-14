/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "extensions/fastfinality/src/WeightedVotingService.cpp"

namespace catapult { namespace mocks {
	constexpr auto Service_Name = "mockweightedvoting";

	class MockWeightedVotingFsm {
	public:
		int startCounter = 0;
		int setPeerConnectionTasksCounter = 0;

		MockWeightedVotingFsm() {}

		void start() {
			startCounter++;
		}
		void setPeerConnectionTasks(extensions::ServiceState& state) {
			setPeerConnectionTasksCounter++;
		}
	};

	std::shared_ptr<MockWeightedVotingFsm> GetMockWeightedVotingFsm(const extensions::ServiceLocator& locator) {
		return locator.service<MockWeightedVotingFsm>(Service_Name);
	}

	class MockWeightedVotingStartServiceRegistrar : public fastfinality::WeightedVotingStartServiceRegistrar {
	public:
		extensions::ServiceRegistrarInfo info() const override {
			return WeightedVotingStartServiceRegistrar::info();
		}
		void registerServiceCounters(extensions::ServiceLocator& locator) override {
			WeightedVotingStartServiceRegistrar::registerServiceCounters(locator);
		}
		void registerServices(extensions::ServiceLocator& locator, extensions::ServiceState& state) override {
			auto pFsmShared = GetMockWeightedVotingFsm(locator);

			pFsmShared->setPeerConnectionTasks(state);
			pFsmShared->start();
		}
	};

}}
