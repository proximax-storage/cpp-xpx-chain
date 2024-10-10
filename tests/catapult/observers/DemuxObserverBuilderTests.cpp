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

#include "catapult/observers/DemuxObserverBuilder.h"
#include "tests/catapult/observers/test/MockTaggedBreadcrumbObserver.h"
#include "tests/test/other/mocks/MockNotification.h"
#include "tests/test/other/mocks/MockNotificationObserver.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS DemuxObserverBuilderTests

	namespace {
		struct TestContext {
		public:
			std::vector<uint16_t> Breadcrumbs;
			AggregateNotificationObserverPointerT<model::Notification> pDemuxObserver;

		public:
			void notify(uint8_t notificationId, NotifyMode mode) {
				state::CatapultState state;
				cache::CatapultCache cache({});
				auto cacheDelta = cache.createDelta();
				auto config = config::BlockchainConfiguration::Uninitialized();
				std::vector<std::unique_ptr<model::Notification>> notifications;
				ObserverState observerState(cacheDelta, state, notifications);
				auto context = test::CreateObserverContext(observerState, config, Height(123), mode);
				test::ObserveNotification<model::Notification>(*pDemuxObserver, test::TaggedNotification(notificationId), context);
			}
		};

		std::unique_ptr<TestContext> CreateTestContext(size_t numObservers, bool varyObservers = false) {
			auto pContext = std::make_unique<TestContext>();
			DemuxObserverBuilder builder;
			for (auto i = 0u; i < numObservers; ++i) {
				auto id = static_cast<uint8_t>(i + 1);
				if (!varyObservers || 1 == id % 2)
					builder.add(mocks::CreateTaggedBreadcrumbObserver(id, pContext->Breadcrumbs));
				else
					builder.add(mocks::CreateTaggedBreadcrumbObserver2(id, pContext->Breadcrumbs));
			}

			auto pDemuxObserver = builder.build();
			pContext->pDemuxObserver = std::move(pDemuxObserver);
			return pContext;
		}
	}

	// region basic delegation

	TEST(TEST_CLASS, CanCreateEmptyDemuxObserver) {
		// Act:
		auto pContext = CreateTestContext(0);
		pContext->notify(12, NotifyMode::Commit);

		// Assert:
		std::vector<std::string> expectedNames;
		EXPECT_EQ(0u, pContext->Breadcrumbs.size());
		EXPECT_EQ("{}", pContext->pDemuxObserver->name());
		EXPECT_EQ(expectedNames, pContext->pDemuxObserver->names());
	}

	TEST(TEST_CLASS, CanCreateDemuxObserverWithMultipleObservers) {
		// Act:
		auto pContext = CreateTestContext(10);
		pContext->notify(12, NotifyMode::Commit);

		// Assert:
		std::vector<std::string> expectedNames{ "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };
		EXPECT_EQ(10u, pContext->Breadcrumbs.size());
		EXPECT_EQ("{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }", pContext->pDemuxObserver->name());
		EXPECT_EQ(expectedNames, pContext->pDemuxObserver->names());
	}

	TEST(TEST_CLASS, AddAllowsChaining) {
		// Arrange:
		auto pContext = std::make_unique<TestContext>();
		DemuxObserverBuilder builder;

		// Act:
		builder
			.add(mocks::CreateTaggedBreadcrumbObserver(2, pContext->Breadcrumbs))
			.add(mocks::CreateTaggedBreadcrumbObserver2(3, pContext->Breadcrumbs))
			.add(mocks::CreateTaggedBreadcrumbObserver(4, pContext->Breadcrumbs));
		pContext->pDemuxObserver = builder.build();

		// Act:
		pContext->notify(7, NotifyMode::Commit);

		// Assert:
		std::vector<uint16_t> expectedBreadcrumbs{ 0x0702, 0x0704 };
		EXPECT_EQ(2u, pContext->Breadcrumbs.size());
		EXPECT_EQ(expectedBreadcrumbs, pContext->Breadcrumbs);
	}

	// endregion

	// region commit + rollback

	TEST(TEST_CLASS, DemuxObserverNotifiesMatchingObserversOnCommit) {
		// Act:
		auto pContext = CreateTestContext(5, true);
		pContext->notify(4, NotifyMode::Commit);

		// Assert:
		std::vector<uint16_t> expectedBreadcrumbs{ 0x0401, 0x0403, 0x0405 };
		EXPECT_EQ(3u, pContext->Breadcrumbs.size());
		EXPECT_EQ(expectedBreadcrumbs, pContext->Breadcrumbs);
	}

	TEST(TEST_CLASS, DemuxObserverNotifiesMatchingObserversOnRollback) {
		// Act:
		auto pContext = CreateTestContext(5, true);
		pContext->notify(4, NotifyMode::Rollback);

		// Assert:
		std::vector<uint16_t> expectedBreadcrumbs{ 0x0405, 0x0403, 0x0401 };
		EXPECT_EQ(3u, pContext->Breadcrumbs.size());
		EXPECT_EQ(expectedBreadcrumbs, pContext->Breadcrumbs);
	}

	TEST(TEST_CLASS, DemuxObserverCanSendMultipleNotifications) {
		// Act:
		auto pContext = CreateTestContext(3, true);
		pContext->notify(4, NotifyMode::Commit);
		pContext->notify(2, NotifyMode::Rollback);
		pContext->notify(5, NotifyMode::Rollback);
		pContext->notify(3, NotifyMode::Commit);

		// Assert:
		std::vector<uint16_t> expectedBreadcrumbs{
			0x0401, 0x0403,
			0x0203, 0x0201,
			0x0503, 0x0501,
			0x0301, 0x0303
		};
		EXPECT_EQ(8u, pContext->Breadcrumbs.size());
		EXPECT_EQ(expectedBreadcrumbs, pContext->Breadcrumbs);
	}

	// endregion

	// region forwarding

#define NOTIFICATION(i) mocks::MockNotification<static_cast<model::NotificationType>(i)>
#define OBSERVER(i) mocks::MockNotificationObserverT<NOTIFICATION(i)>

#define ADD_OBSERVER(i) { \
        std::unique_ptr<const NotificationObserverT<NOTIFICATION(i)>> pMockObserver = std::make_unique<OBSERVER(i)>(std::to_string(i)); \
        observers.push_back(pMockObserver.get()); \
        builder.add(std::move(pMockObserver)); \
	}

#define ASSERT_OBSERVER(i, GET_VALUES, EXPECTED_VALUE) { \
		const auto& values = static_cast<const OBSERVER(i)*>(observers[i])->GET_VALUES(); \
		const auto message = "observer at " + std::to_string(i); \
		if (2 == i) { \
			ASSERT_EQ(1u, values.size()) << message; \
			EXPECT_EQ(EXPECTED_VALUE, values[0]) << message; \
		} else { \
			EXPECT_EQ(0u, values.size()) << message; \
		} \
	}

	template<typename TBuilder>
	std::vector<const void*> AddSubObservers(TBuilder& builder) {
		std::vector<const void*> observers;
		ADD_OBSERVER(0)
		ADD_OBSERVER(1)
		ADD_OBSERVER(2)
		ADD_OBSERVER(3)
		ADD_OBSERVER(4)

		return observers;
	}

	/// Asserts that the observer created by \a builder delegates to all sub observers and passes notifications correctly.
	template<typename TAssertFunc>
	void AssertChildObservers(TAssertFunc assertFunc) {
		// Arrange:
		state::CatapultState state;
		cache::CatapultCache cache({});
		auto cacheDelta = cache.createDelta();
		auto config = config::BlockchainConfiguration::Uninitialized();
		std::vector<std::unique_ptr<model::Notification>> notifications;
		ObserverState observerState(cacheDelta, state, notifications);
		auto context = test::CreateObserverContext(observerState, config, Height(123), observers::NotifyMode::Commit);

		// - create an aggregate with five observers
		DemuxObserverBuilder builder;
		auto observers = AddSubObservers(builder);
		auto pAggregateObserver = builder.build();

		// - create two notifications
		auto notification1 = mocks::MockNotification<static_cast<model::NotificationType>(7)>();
		auto notification2 = mocks::MockNotification<static_cast<model::NotificationType>(2)>();

		// Act:
		pAggregateObserver->notify(notification1, context);
		pAggregateObserver->notify(notification2, context);

		// Assert:
		assertFunc(observers, notification2, context);
	}

	TEST(TEST_CLASS, NotificationsAreForwardedToChildObservers) {
		// Assert:
		AssertChildObservers([](const auto& observers, const auto& notification, const auto&) {
			ASSERT_OBSERVER(0, notificationTypes, notification.Type)
			ASSERT_OBSERVER(1, notificationTypes, notification.Type)
			ASSERT_OBSERVER(2, notificationTypes, notification.Type)
			ASSERT_OBSERVER(3, notificationTypes, notification.Type)
			ASSERT_OBSERVER(4, notificationTypes, notification.Type)
		});
	}

	TEST(TEST_CLASS, ContextsAreForwardedToChildObservers) {
		// Assert:
		AssertChildObservers([](const auto& observers, const auto&, const auto& context) {
			ASSERT_OBSERVER(0, contextPointers, &context)
			ASSERT_OBSERVER(1, contextPointers, &context)
			ASSERT_OBSERVER(2, contextPointers, &context)
			ASSERT_OBSERVER(3, contextPointers, &context)
			ASSERT_OBSERVER(4, contextPointers, &context)
		});
	}

	// endregion

	// region filtering

	namespace {
		using Breadcrumbs = std::vector<std::string>;

		template<typename TNotification>
		class MockBreadcrumbObserver : public NotificationObserverT<TNotification> {
		public:
			MockBreadcrumbObserver(const std::string& name, Breadcrumbs& breadcrumbs)
					: m_name(name)
					, m_breadcrumbs(breadcrumbs)
			{}

		public:
			const std::string& name() const override {
				return m_name;
			}

			void notify(const TNotification&, ObserverContext&) const override {
				m_breadcrumbs.push_back(m_name);
			}

		private:
			std::string m_name;
			Breadcrumbs& m_breadcrumbs;
		};

		template<typename TNotification = model::Notification>
		NotificationObserverPointerT<TNotification> CreateBreadcrumbObserver(Breadcrumbs& breadcrumbs, const std::string& name) {
			return std::make_unique<MockBreadcrumbObserver<TNotification>>(name, breadcrumbs);
		}
	}

	TEST(TEST_CLASS, CanFilterObserversBasedOnNotificationType) {
			// Arrange:
		Breadcrumbs breadcrumbs;
		DemuxObserverBuilder builder;

		state::CatapultState state;
		cache::CatapultCache cache({});
		auto cacheDelta = cache.createDelta();
		auto config = config::BlockchainConfiguration::Uninitialized();
		std::vector<std::unique_ptr<model::Notification>> notifications;
		ObserverState observerState(cacheDelta, state, notifications);
		auto context = test::CreateObserverContext(observerState, config, Height(123), NotifyMode::Commit);

		builder
			.add(CreateBreadcrumbObserver<model::AccountPublicKeyNotification<1>>(breadcrumbs, "alpha"))
			.add(CreateBreadcrumbObserver<model::AccountAddressNotification<1>>(breadcrumbs, "OMEGA"))
			.add(CreateBreadcrumbObserver<model::BalanceTransferNotification<1>>(breadcrumbs, "zEtA"));
		auto pObserver = builder.build();

		// Act:
		auto notification = model::AccountPublicKeyNotification<1>(Key());
		test::ObserveNotification<model::Notification>(*pObserver, notification, context);

		// Assert:
		Breadcrumbs expectedNames{ "alpha", "OMEGA", "zEtA" };
		EXPECT_EQ(expectedNames, pObserver->names());

		// - alpha matches notification type
		// - OMEGA does not match notification type
		// - zEtA does not match notification type
		Breadcrumbs expectedSelectedNames{ "alpha" };
		EXPECT_EQ(expectedSelectedNames, breadcrumbs);
	}

	// endregion
}}
