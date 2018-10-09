/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "mongo/src/ApiStateChangeSubscriber.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo {

#define TEST_CLASS ApiStateChangeSubscriberTests

	namespace {
		// region basic mocks

		class MockExternalCacheStorage : public ExternalCacheStorage {
		public:
			MockExternalCacheStorage() : ExternalCacheStorage("MockExternalCacheStorage", std::numeric_limits<size_t>::max())
			{}

		public:
			const auto& deltas() const {
				return m_deltas;
			}

		public:
			void saveDelta(const cache::CatapultCacheDelta& cache) override {
				m_deltas.push_back(&cache);
			}

			void loadAll(cache::CatapultCache&, Height) const override {
				CATAPULT_THROW_RUNTIME_ERROR("loadAll - not supported in mock");
			}

		private:
			std::vector<const cache::CatapultCacheDelta*> m_deltas;
		};

		// endregion

		class TestContext {
		public:
			TestContext()
					: m_pExternalCacheStorage(std::make_unique<MockExternalCacheStorage>())
					, m_pExternalCacheStorageRaw(m_pExternalCacheStorage.get())
					, m_subscriber(std::move(m_pExternalCacheStorage))
			{}

		public:
			auto& externalCacheStorage() {
				return *m_pExternalCacheStorageRaw;
			}

			auto& subscriber() {
				return m_subscriber;
			}

		private:
			std::unique_ptr<MockExternalCacheStorage> m_pExternalCacheStorage; // notice that this is moved into m_subscriber
			MockExternalCacheStorage* m_pExternalCacheStorageRaw;
			ApiStateChangeSubscriber m_subscriber;
		};
	}

	TEST(TEST_CLASS, NotifyStateChangeForwardsToExternalCacheStorage) {
		// Arrange:
		TestContext context;
		auto cache = cache::CatapultCache({});
		auto cacheDelta = cache.createDelta();

		// Act:
		context.subscriber().notifyStateChange(consumers::StateChangeInfo(cacheDelta, Height(123)));

		// Assert:
		ASSERT_EQ(1u, context.externalCacheStorage().deltas().size());
		EXPECT_EQ(&cacheDelta, context.externalCacheStorage().deltas()[0]);
	}
}}
