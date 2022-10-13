/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/txes/contract_v2/src/model/ContractNotifications.h"
#include "src/plugins/DeployTransactionPlugin.h"
#include "src/model/DeployTransaction.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/ContractTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS DeployTransactionPluginTests

    namespace {
       	constexpr auto File_Name_Size = 10u;
        constexpr auto Function_Name_Size = 9u;
        constexpr auto Actual_Arguments_Size = 8u;
        constexpr auto Service_Payment_Count = 3u;
        constexpr auto Automated_Execution_File_Name_Size = 7u;
        constexpr auto Automated_Execution_Function_Name_Size = 6u;

    	DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(Deploy, 1, 1,)

        template<typename TTraits>
		auto CreateTransaction(uint8_t servicePaymentCount) {
			return test::CreateDeployTransaction<typename TTraits::TransactionType>(Service_Payment_Count);
		}
    }

    DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_Deploy)

    PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>(2);
		pTransaction->FileNameSize = File_Name_Size;
        pTransaction->FunctionNameSize = Function_Name_Size;
        pTransaction->ActualArgumentsSize = Actual_Arguments_Size;
        pTransaction->ServicePaymentCount = Service_Payment_Count;
        pTransaction->AutomatedExecutionFileNameSize = Automated_Execution_File_Name_Size;
        pTransaction->AutomatedExecutionFunctionNameSize = Automated_Execution_Function_Name_Size;

		// Act:
		auto realSize = pPlugin->calculateRealSize(*pTransaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) 
                + File_Name_Size + Function_Name_Size + Actual_Arguments_Size
                + Service_Payment_Count * sizeof(Mosaic)
                + Automated_Execution_File_Name_Size + Automated_Execution_Function_Name_Size, realSize);
	}

    // region publish - basic

    PLUGIN_TEST(PublishesNoNotificationWhenTransactionVersionIsInvalid) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(transaction);
		transaction.Version = MakeVersion(NetworkIdentifier::Mijin_Test, std::numeric_limits<uint32_t>::max());

		// Act:
		test::PublishTransaction(*pPlugin, transaction, sub);

		// Assert:
		ASSERT_EQ(0, sub.numNotifications());
	}

    PLUGIN_TEST(CanPublishCorrectNumberOfNotifications) {
		// Arrange:
		auto pTransaction = CreateTransaction<TTraits>(2);
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numNotifications());
		auto i = 0u;
		EXPECT_EQ(Contract_v2_Deploy_v1_Notification, sub.notificationTypes()[i++]);
	}

    // endregion

	// region publish - deploy notification

	PLUGIN_TEST(CanPublishDeployNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<DeployNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>(2);

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
		EXPECT_EQ(pTransaction->FileNameSize, notification.FileName.size());
		EXPECT_EQ(pTransaction->FunctionNameSize, notification.FunctionName.size());
		EXPECT_EQ(pTransaction->ActualArgumentsSize, notification.ActualArguments.size());
		EXPECT_EQ(pTransaction->ExecutionCallPayment, notification.ExecutionCallPayment);
		EXPECT_EQ(pTransaction->DownloadCallPayment, notification.DownloadCallPayment);
		EXPECT_EQ(1u, notification.ServicePaymentCount);
		EXPECT_EQ(1u, notification.SingleApprovement);
		EXPECT_EQ(pTransaction->AutomatedExecutionFileNameSize, notification.AutomatedExecutionFileName.size());
		EXPECT_EQ(pTransaction->AutomatedExecutionFunctionNameSize, notification.AutomatedExecutionFunctionName.size());
		EXPECT_EQ(pTransaction->AutomatedExecutionCallPayment, notification.AutomatedExecutionCallPayment);
		EXPECT_EQ(pTransaction->AutomatedDownloadCallPayment, notification.AutomatedDownloadCallPayment);
		EXPECT_EQ(pTransaction->AutomatedExecutionsNumber, notification.AutomatedExecutionsNumber);
		EXPECT_EQ(pTransaction->Assignee, notification.Assignee);
		EXPECT_EQ_MEMORY(pTransaction->FileNamePtr(), notification.FileName.data(), notification.FileName.size());
		EXPECT_EQ_MEMORY(pTransaction->FunctionNamePtr(), notification.FunctionName.data(), notification.FunctionName.size());
		EXPECT_EQ_MEMORY(pTransaction->ActualArgumentsPtr(), notification.ActualArguments.data(), notification.ActualArguments.size());
		EXPECT_EQ_MEMORY(pTransaction->ServicePaymentPtr(), notification.ServicePaymentPtr, pTransaction->ServicePaymentCount * sizeof(UnresolvedMosaic));
		EXPECT_EQ_MEMORY(pTransaction->AutomatedExecutionFileNamePtr(), notification.AutomatedExecutionFileName.data(), notification.AutomatedExecutionFileName.size());
		EXPECT_EQ_MEMORY(pTransaction->AutomatedExecutionFunctionNamePtr(), notification.AutomatedExecutionFunctionName.data(), notification.AutomatedExecutionFunctionName.size());
	}

    // endregion
}}