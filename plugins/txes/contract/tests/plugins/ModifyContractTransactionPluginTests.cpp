/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/utils/MemoryUtils.h"
#include "plugins/txes/multisig/src/model/MultisigNotifications.h"
#include "plugins/txes/contract/src/model/ContractNotifications.h"
#include "src/plugins/ModifyContractTransactionPlugin.h"
#include "src/model/ModifyContractTransaction.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/TestHarness.h"
#include <random>

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS ModifyContractTransactionPluginTests

	namespace {
		constexpr auto Transaction_Version = MakeVersion(NetworkIdentifier::Mijin_Test, 3);

		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(ModifyContract, 3, 3,)

		CosignatoryModification GenerateModification(int index) {
			return { ((index % 2) ? CosignatoryModificationType::Del : CosignatoryModificationType::Add),
				 test::GenerateRandomByteArray<Key>() };
		}

		template<typename TTraits>
		auto CreateTransactionWithModifications(
				uint8_t customerModificationCount,
				uint8_t executorModificationCount,
				uint8_t verifierModificationCount,
				bool generateModifications = true) {
			using TransactionType = typename TTraits::TransactionType;
			uint32_t entitySize = sizeof(TransactionType)
				+ customerModificationCount * sizeof(CosignatoryModification)
				+ executorModificationCount * sizeof(CosignatoryModification)
				+ verifierModificationCount * sizeof(CosignatoryModification);
			auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
			pTransaction->Version = Transaction_Version;
			pTransaction->Size = entitySize;
			pTransaction->CustomerModificationCount = customerModificationCount;
			pTransaction->ExecutorModificationCount = executorModificationCount;
			pTransaction->VerifierModificationCount = verifierModificationCount;
			test::FillWithRandomData(pTransaction->Signer);
			pTransaction->Type = static_cast<model::EntityType>(0x0815);
			if (generateModifications) {
				auto *pModification = pTransaction->ExecutorModificationsPtr();
				for (int i = 0; i < executorModificationCount; ++i)
					*pModification++ = GenerateModification(i);
				pModification = pTransaction->VerifierModificationsPtr();
				for (int i = 0; i < verifierModificationCount; ++i)
					*pModification++ = GenerateModification(i);
			}
			return pTransaction;
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_Modify_Contract)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;
		transaction.Size = 0;
		transaction.CustomerModificationCount = 7;
		transaction.ExecutorModificationCount = 8;
		transaction.VerifierModificationCount = 9;

		// Act:
		auto realSize = pPlugin->calculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + 24 * sizeof(CosignatoryModification), realSize);
	}

	// region publish - basic

	namespace {
		template<typename TTraits, typename TAdditionalAssert>
		void AssertNumNotifications(
				size_t numExpectedNotifications,
				const typename TTraits::TransactionType& transaction,
				TAdditionalAssert additionalAssert) {
			// Arrange:
			mocks::MockNotificationSubscriber sub;
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			test::PublishTransaction(*pPlugin, transaction, sub);

			// Assert:
			ASSERT_EQ(numExpectedNotifications, sub.numNotifications());
			additionalAssert(sub);
		}

		template<typename TTraits>
		void AssertNumNotifications(size_t numExpectedNotifications, const typename TTraits::TransactionType& transaction) {
			// Assert:
			AssertNumNotifications<TTraits>(numExpectedNotifications, transaction, [](const auto&) {});
		}
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotificationsWhenNoModificationsArePresent) {
		// Arrange:
		auto pTransaction = CreateTransactionWithModifications<TTraits>(0, 0, 0, false);

		// Assert:
		AssertNumNotifications<TTraits>(1, *pTransaction);
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotificationsWhenNoAddModificationsArePresent) {
		// Arrange:
		auto pTransaction = CreateTransactionWithModifications<TTraits>(3, 2, 3, false);
		auto* pModification = pTransaction->ExecutorModificationsPtr();
		*pModification++ = { CosignatoryModificationType::Del, test::GenerateRandomByteArray<Key>() };
		*pModification++ = { CosignatoryModificationType::Del, test::GenerateRandomByteArray<Key>() };
		pModification = pTransaction->VerifierModificationsPtr();
		*pModification++ = { CosignatoryModificationType::Del, test::GenerateRandomByteArray<Key>() };
		*pModification++ = { CosignatoryModificationType::Del, test::GenerateRandomByteArray<Key>() };
		*pModification++ = { CosignatoryModificationType::Del, test::GenerateRandomByteArray<Key>() };

		// Assert:
		AssertNumNotifications<TTraits>(3 + 3, *pTransaction);
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotificationsWhenAddModificationsArePresent) {
		// Arrange:
		auto pTransaction = CreateTransactionWithModifications<TTraits>(3, 2, 3);

		// Assert:
		AssertNumNotifications<TTraits>(6 + 3, *pTransaction, [](const auto& sub) {
			// - multisig modify new cosigner notifications must be the first raised notifications
			EXPECT_EQ(Multisig_Modify_New_Cosigner_v1_Notification, sub.notificationTypes()[0]);
			EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[1]);
			EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[2]);
			EXPECT_EQ(Multisig_Modify_New_Cosigner_v1_Notification, sub.notificationTypes()[3]);
			EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[4]);
		});
	}

	// endregion

	// region publish - modify contract notification

	PLUGIN_TEST(CanPublishModifyContractNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ModifyContractNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();

		auto pTransaction = CreateTransactionWithModifications<TTraits>(0, 0, 0, false);
		pTransaction->DurationDelta = 1000;
		pTransaction->Signer = test::GenerateRandomByteArray<Key>();
		pTransaction->Hash = test::GenerateRandomByteArray<Hash256>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(1000, notification.DurationDelta);
		EXPECT_EQ(pTransaction->Signer, notification.Multisig);
		EXPECT_EQ(pTransaction->Hash, notification.Hash);
	}

	// endregion

	// region publish - modify multisig cosigners notification

	PLUGIN_TEST(CanPublishCosignersNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ModifyMultisigCosignersNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();

		auto pTransaction = CreateTransactionWithModifications<TTraits>(3, 2, 3);

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.Signer);
		EXPECT_EQ(3, notification.ModificationsCount);
		EXPECT_EQ(pTransaction->VerifierModificationsPtr(), notification.ModificationsPtr);
	}

	PLUGIN_TEST(NoCosignersNotificationIfNoModificationIsPresent) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ModifyMultisigCosignersNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();

		auto pTransaction = CreateTransactionWithModifications<TTraits>(0, 0, 0, false);

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numNotifications());
		ASSERT_EQ(0u, sub.numMatchingNotifications());
	}

	// endregion

	// region publish - modify multisig new cosigner notification

	PLUGIN_TEST(CanPublishNewCosignerNotificationForEachAddVerifierModification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ModifyMultisigNewCosignerNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();

		auto pTransaction = CreateTransactionWithModifications<TTraits>(3, 2, 3);

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(2u, sub.numMatchingNotifications());

		EXPECT_EQ(pTransaction->Signer, sub.matchingNotifications()[0].MultisigAccountKey);
		EXPECT_EQ(pTransaction->VerifierModificationsPtr()[0].CosignatoryPublicKey, sub.matchingNotifications()[0].CosignatoryKey);

		EXPECT_EQ(pTransaction->Signer, sub.matchingNotifications()[1].MultisigAccountKey);
		EXPECT_EQ(pTransaction->VerifierModificationsPtr()[2].CosignatoryPublicKey, sub.matchingNotifications()[1].CosignatoryKey);
	}

	PLUGIN_TEST(NoNewCosignerNotificationsIfNoAddModificationsArePresent) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ModifyMultisigNewCosignerNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();

		auto pTransaction = CreateTransactionWithModifications<TTraits>(3, 2, 3, false);
		auto* pModification = pTransaction->VerifierModificationsPtr();
		*pModification++ = { CosignatoryModificationType::Del, test::GenerateRandomByteArray<Key>() };
		*pModification++ = { CosignatoryModificationType::Del, test::GenerateRandomByteArray<Key>() };
		*pModification++ = { CosignatoryModificationType::Del, test::GenerateRandomByteArray<Key>() };

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(3u + 3u, sub.numNotifications());
		ASSERT_EQ(0u, sub.numMatchingNotifications());
	}

	// endregion

	// region publish - reputation update notification

	PLUGIN_TEST(CanPublishReputationUpdateNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ReputationUpdateNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();

		auto pTransaction = CreateTransactionWithModifications<TTraits>(3, 2, 3);

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(5, notification.ModificationCount());
	}

	PLUGIN_TEST(NoReputationUpdateNotificationIfNoModificationIsPresent) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ReputationUpdateNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();

		auto pTransaction = CreateTransactionWithModifications<TTraits>(0, 0, 0, false);

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numNotifications());
		ASSERT_EQ(0u, sub.numMatchingNotifications());
	}

	// endregion

	// region address interaction

	namespace {
		utils::KeySet ExtractAddedModificationKeys(const model::CosignatoryModification* pModifications, size_t modificationsCount) {
			utils::KeySet keys;
			for (auto i = 0u; i < modificationsCount; ++i) {
				if (CosignatoryModificationType::Add == pModifications[i].ModificationType)
					keys.insert(pModifications[i].CosignatoryPublicKey);
			}

			return keys;
		}

		std::vector<CosignatoryModificationType> GenerateRandomModificationTypeSequence(size_t numAdds, size_t numDels) {
			std::vector<CosignatoryModificationType> modificationTypes;
			for (auto i = 0u; i < numAdds + numDels; ++i)
				modificationTypes.push_back(i < numAdds ? CosignatoryModificationType::Add : CosignatoryModificationType::Del);

			std::mt19937 generator((std::random_device()()));
			std::shuffle(modificationTypes.begin(), modificationTypes.end(), generator);
			return modificationTypes;
		}

		template<typename TTraits>
		void AssertAddressInteractionNotifications(size_t numAddModifications, size_t numDelModifications) {
			// Arrange:
			mocks::MockTypedNotificationSubscriber<AddressInteractionNotification<1>> sub;
			auto pPlugin = TTraits::CreatePlugin();

			auto numModifications = static_cast<uint8_t>(numAddModifications + numDelModifications);
			auto pTransaction = CreateTransactionWithModifications<TTraits>(0, 0, numModifications, false);
			auto modificationTypes = GenerateRandomModificationTypeSequence(numAddModifications, numDelModifications);
			auto* pModification = pTransaction->VerifierModificationsPtr();
			for (auto modificationType : modificationTypes)
				*pModification++ = { modificationType, test::GenerateRandomByteArray<Key>() };

			// Act:
			test::PublishTransaction(*pPlugin, *pTransaction, sub);

			// Assert:
			ASSERT_EQ(0 < numAddModifications ? 1u : 0u, sub.numMatchingNotifications());
			if (0 < numAddModifications) {
				pModification = pTransaction->VerifierModificationsPtr();
				auto addedKeys = ExtractAddedModificationKeys(pModification, numAddModifications + numDelModifications);
				const auto& notification = sub.matchingNotifications()[0];
				EXPECT_EQ(pTransaction->Signer, notification.Source);
				EXPECT_EQ(pTransaction->Type, notification.TransactionType);
				EXPECT_EQ(model::UnresolvedAddressSet{}, notification.ParticipantsByAddress);
				EXPECT_EQ(addedKeys, notification.ParticipantsByKey);
			}
		}
	}

	PLUGIN_TEST(NoAddressInteractionNotificationWhenNoAddModificationsArePresent) {
		// Assert:
		AssertAddressInteractionNotifications<TTraits>(0, 0);
		AssertAddressInteractionNotifications<TTraits>(0, 3);
	}

	PLUGIN_TEST(CanPublishAddressInteractionNotificationWhenAddModificationsArePresent) {
		// Assert:
		AssertAddressInteractionNotifications<TTraits>(4, 0);
		AssertAddressInteractionNotifications<TTraits>(4, 3);
	}

	// endregion
}}
