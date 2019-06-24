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

#include "src/observers/Observers.h"
#include "src/model/SecretLockReceiptType.h"
#include "plugins/txes/lock_shared/tests/observers/LockObserverTests.h"
#include "tests/test/SecretLockInfoCacheTestUtils.h"
#include "tests/test/SecretLockNotificationsTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS SecretLockObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(SecretLock,)

	namespace {
		struct SecretObserverTraits {
		public:
			using CacheType = cache::SecretLockInfoCache;
			using NotificationType = model::SecretLockNotification;
			using NotificationBuilder = test::SecretLockNotificationBuilder;
			using ObserverTestContext = test::ObserverTestContextT<test::SecretLockInfoCacheFactory>;

			static constexpr auto Debit_Receipt_Type = model::Receipt_Type_LockSecret_Created;

			static auto CreateObserver() {
				return CreateSecretLockObserver();
			}

			static auto GenerateRandomLockInfo(const NotificationType& notification) {
				auto resolver = test::CreateResolverContextXor();
				auto lockInfo = test::BasicSecretLockInfoTestTraits::CreateLockInfo();
				lockInfo.Secret = notification.Secret;
				lockInfo.Recipient = resolver.resolve(notification.Recipient);
				lockInfo.CompositeHash = model::CalculateSecretLockInfoHash(lockInfo.Secret, lockInfo.Recipient);
				return lockInfo;
			}

			static auto ToKey(const NotificationType& notification) {
				auto resolver = test::CreateResolverContextXor();
				return model::CalculateSecretLockInfoHash(notification.Secret, resolver.resolve(notification.Recipient));
			}

			static void AssertAddedLockInfo(const state::SecretLockInfo& lockInfo, const NotificationType& notification) {
				// Assert:
				EXPECT_EQ(notification.HashAlgorithm, lockInfo.HashAlgorithm);
				EXPECT_EQ(notification.Secret, lockInfo.Secret);
				EXPECT_EQ(notification.Recipient, test::UnresolveXor(lockInfo.Recipient));
			}
		};
	}

	DEFINE_LOCK_OBSERVER_TESTS(SecretObserverTraits)
}}
