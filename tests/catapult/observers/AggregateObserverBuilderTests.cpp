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

#include "catapult/observers/AggregateObserverBuilder.h"
#include "tests/catapult/observers/test/AggregateObserverTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS AggregateObserverBuilderTests

	namespace {
		struct TestContext {
		public:
			std::vector<uint16_t> Breadcrumbs;
			AggregateNotificationObserverPointerT<test::TaggedNotification> pAggregateObserver;

		public:
			void notify(uint8_t notificationId, NotifyMode mode) {
				auto cache = test::CreateEmptyCatapultCache();
				auto cacheDelta = cache.createDelta();
				state::CatapultState state;
				auto config = config::BlockchainConfiguration::Uninitialized();
				std::vector<std::unique_ptr<model::Notification>> notifications;
				ObserverState observerState(cacheDelta, state, notifications);
				auto context = test::CreateObserverContext(observerState, config, Height(123), mode);
				test::ObserveNotification(*pAggregateObserver, test::TaggedNotification(notificationId), context);
			}
		};

		std::unique_ptr<TestContext> CreateTestContext(size_t numObservers) {
			auto pContext = std::make_unique<TestContext>();
			AggregateObserverBuilder<test::TaggedNotification> builder;
			for (auto i = 0u; i < numObservers; ++i)
				builder.add(test::TaggedNotification::Notification_Type, mocks::CreateTaggedBreadcrumbObserver(static_cast<uint8_t>(i + 1), pContext->Breadcrumbs));

			pContext->pAggregateObserver = builder.build();
			return pContext;
		}
	}

	// region basic delegation

	TEST(TEST_CLASS, CanCreateEmptyAggregateObserver) {
		// Act:
		auto pContext = CreateTestContext(0);
		pContext->notify(7, NotifyMode::Commit);

		// Assert:
		std::vector<std::string> expectedNames;
		EXPECT_EQ(0u, pContext->Breadcrumbs.size());
		EXPECT_EQ("{}", pContext->pAggregateObserver->name());
		EXPECT_EQ(expectedNames, pContext->pAggregateObserver->names());
	}

	TEST(TEST_CLASS, CanCreateAggregateObserverWithMultipleObservers) {
		// Act:
		auto pContext = CreateTestContext(10);
		pContext->notify(7, NotifyMode::Commit);

		// Assert:
		std::vector<std::string> expectedNames{ "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };
		EXPECT_EQ(10u, pContext->Breadcrumbs.size());
		EXPECT_EQ("{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }", pContext->pAggregateObserver->name());
		EXPECT_EQ(expectedNames, pContext->pAggregateObserver->names());
	}

	TEST(TEST_CLASS, AddAllowsChaining) {
		// Act:
		auto pContext = std::make_unique<TestContext>();
		AggregateObserverBuilder<test::TaggedNotification> builder;
		builder
			.add(test::TaggedNotification::Notification_Type, mocks::CreateTaggedBreadcrumbObserver(2, pContext->Breadcrumbs))
			.add(test::TaggedNotification::Notification_Type, mocks::CreateTaggedBreadcrumbObserver(3, pContext->Breadcrumbs))
			.add(test::TaggedNotification::Notification_Type, mocks::CreateTaggedBreadcrumbObserver(4, pContext->Breadcrumbs));
		pContext->pAggregateObserver = builder.build();

		// Act:
		pContext->notify(7, NotifyMode::Commit);

		// Assert:
		std::vector<uint16_t> expectedBreadcrumbs{ 0x0702, 0x0703, 0x0704 };
		EXPECT_EQ(3u, pContext->Breadcrumbs.size());
		EXPECT_EQ(expectedBreadcrumbs, pContext->Breadcrumbs);
	}

	// endregion

	// region commit + rollback

	TEST(TEST_CLASS, AggregateObserverNotifiesAllObserversOnCommit) {
		// Act:
		auto pContext = CreateTestContext(5);
		pContext->notify(7, NotifyMode::Commit);

		// Assert:
		std::vector<uint16_t> expectedBreadcrumbs{ 0x0701, 0x0702, 0x0703, 0x0704, 0x0705 };
		EXPECT_EQ(5u, pContext->Breadcrumbs.size());
		EXPECT_EQ(expectedBreadcrumbs, pContext->Breadcrumbs);
	}

	TEST(TEST_CLASS, AggregateObserverNotifiesAllObserversOnRollback) {
		// Act:
		auto pContext = CreateTestContext(5);
		pContext->notify(7, NotifyMode::Rollback);

		// Assert:
		std::vector<uint16_t> expectedBreadcrumbs{ 0x0705, 0x0704, 0x0703, 0x0702, 0x0701 };
		EXPECT_EQ(5u, pContext->Breadcrumbs.size());
		EXPECT_EQ(expectedBreadcrumbs, pContext->Breadcrumbs);
	}

	TEST(TEST_CLASS, AggregateObserverCanSendMultipleNotifications) {
		// Act:
		auto pContext = CreateTestContext(3);
		pContext->notify(2, NotifyMode::Commit);
		pContext->notify(7, NotifyMode::Rollback);
		pContext->notify(5, NotifyMode::Rollback);
		pContext->notify(1, NotifyMode::Commit);

		// Assert:
		std::vector<uint16_t> expectedBreadcrumbs{
			0x0201, 0x0202, 0x0203,
			0x0703, 0x0702, 0x0701,
			0x0503, 0x0502, 0x0501,
			0x0101, 0x0102, 0x0103,
		};
		EXPECT_EQ(12u, pContext->Breadcrumbs.size());
		EXPECT_EQ(expectedBreadcrumbs, pContext->Breadcrumbs);
	}

	// endregion

	// region forwarding

	TEST(TEST_CLASS, NotificationsAreForwardedToChildObservers) {
		// Assert:
		test::AssertNotificationsAreForwardedToChildObservers(
				AggregateObserverBuilder<model::Notification>(),
				[](auto& builder, auto&& pObserver, auto type) { builder.add(type, std::move(pObserver)); });
	}

	TEST(TEST_CLASS, ContextsAreForwardedToChildObservers) {
		// Assert:
		test::AssertContextsAreForwardedToChildObservers(
				AggregateObserverBuilder<model::Notification>(),
				[](auto& builder, auto&& pObserver, auto type) { builder.add(type, std::move(pObserver)); });
	}

	// endregion
}}
