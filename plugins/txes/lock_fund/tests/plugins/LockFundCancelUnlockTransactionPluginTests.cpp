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

#include "src/plugins/LockFundCancelUnlockTransactionPlugin.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "src/model/LockFundNotifications.h"
#include "src/model/LockFundCancelUnlockTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/TestHarness.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS LockFundCancelUnlockTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(LockFundCancelUnlock, 1, 1,)

		constexpr auto Transaction_Version = MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);

		template<typename TTraits>
		auto CreateTransactionWithMosaics(uint8_t numMosaics, uint16_t messageSize = 0) {
			using TransactionType = typename TTraits::TransactionType;
			uint32_t entitySize = sizeof(TransactionType) + numMosaics * sizeof(Mosaic) + messageSize;
			auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
			pTransaction->Version = Transaction_Version;
			pTransaction->Size = entitySize;
			pTransaction->TargetHeight = Height(100);
			test::FillWithRandomData(pTransaction->Signer);
			return pTransaction;
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , Entity_Type_Lock_Fund_Cancel_Unlock)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;
		transaction.Size = 0;
		transaction.TargetHeight = Height(10);

		// Act:
		auto realSize = pPlugin->calculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType), realSize);
	}

	PLUGIN_TEST(CanExtractAccounts) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(transaction);
		transaction.Version = Transaction_Version;
		transaction.TargetHeight = Height(10);
		test::FillWithRandomData(transaction.Signer);

		// Act:
		test::PublishTransaction(*pPlugin, transaction, sub);

		// Assert:
		EXPECT_EQ(1u, sub.numNotifications());
		EXPECT_EQ(1u, sub.numKeys());
		EXPECT_EQ(0u, sub.numAddresses());

		EXPECT_TRUE(sub.contains(transaction.Signer));
	}
}}
