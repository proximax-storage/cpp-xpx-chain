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

#include "catapult/observers/FunctionalNotificationObserver.h"
#include "tests/test/nodeps/ParamsCapture.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS FunctionalNotificationObserverTests

	using NotificationType = model::AccountPublicKeyNotification<1>;

	TEST(TEST_CLASS, HasCorrectName) {
		// Act:
		FunctionalNotificationObserverT<NotificationType> observer("Foo", [](const auto&, const auto&) {});

		// Assert:
		EXPECT_EQ("Foo", observer.name());
	}

	namespace {
		struct NotifyParams {
		public:
			NotifyParams(const NotificationType& notification, const ObserverContext& context)
					: pNotification(&notification)
					, pContext(&context)
			{}

		public:
			const NotificationType* pNotification;
			const ObserverContext* pContext;
		};
	}

	TEST(TEST_CLASS, NotifyDelegatesToFunction) {
		// Arrange:
		test::ParamsCapture<NotifyParams> capture;
		FunctionalNotificationObserverT<NotificationType> observer("Foo", [&](const auto& notification, const auto& context) {
			capture.push(notification, context);
		});

		// Act:
		state::CatapultState state;
		cache::CatapultCache cache({});
		auto cacheDelta = cache.createDelta();
		auto config = config::BlockchainConfiguration::Uninitialized();
		std::vector<std::unique_ptr<model::Notification>> notifications;
		ObserverState observerState(cacheDelta, state, notifications);
		auto context = test::CreateObserverContext(observerState, config, Height(123), NotifyMode::Commit);

		auto publicKey = test::GenerateRandomByteArray<Key>();
		NotificationType notification(publicKey);
		observer.notify(notification, context);

		// Assert:
		ASSERT_EQ(1u, capture.params().size());
		EXPECT_EQ(&notification, capture.params()[0].pNotification);
		EXPECT_EQ(&context, capture.params()[0].pContext);
	}
}}
