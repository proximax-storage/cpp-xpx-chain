/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/LockFundTransferTransactionPlugin.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "src/model/LockFundNotifications.h"
#include "src/model/LockFundTransferTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/TestHarness.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS TransferTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(LockFundTransfer, 1, 1,)

		constexpr auto Transaction_Version = MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);

		template<typename TTraits>
		auto CreateTransactionWithMosaics(uint8_t numMosaics, uint16_t messageSize = 0) {
			using TransactionType = typename TTraits::TransactionType;
			uint32_t entitySize = sizeof(TransactionType) + numMosaics * sizeof(Mosaic) + messageSize;
			auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
			pTransaction->Version = Transaction_Version;
			pTransaction->Size = entitySize;
			pTransaction->Action = model::LockFundAction::Lock;
			pTransaction->Duration = BlockDuration(200000);
			pTransaction->MosaicsCount = numMosaics;
			test::FillWithRandomData(pTransaction->Signer);
			return pTransaction;
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , Entity_Type_Lock_Fund_Transfer)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;
		transaction.Size = 0;
		transaction.MosaicsCount = 7;

		// Act:
		auto realSize = pPlugin->calculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + 7 * sizeof(Mosaic), realSize);
	}

	PLUGIN_TEST(CanExtractAccounts) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(transaction);
		transaction.Version = Transaction_Version;
		transaction.Action = model::LockFundAction::Lock;
		transaction.Duration = BlockDuration();
		transaction.MosaicsCount = 0;
		test::FillWithRandomData(transaction.Signer);

		// Act:
		test::PublishTransaction(*pPlugin, transaction, sub);

		// Assert:
		EXPECT_EQ(2u, sub.numNotifications());
		EXPECT_EQ(1u, sub.numKeys());
		EXPECT_EQ(0u, sub.numAddresses());

		EXPECT_TRUE(sub.contains(transaction.Signer));
	}
}}
