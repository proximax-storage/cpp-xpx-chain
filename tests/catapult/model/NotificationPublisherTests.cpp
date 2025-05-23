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

#include "catapult/model/NotificationPublisher.h"
#include "catapult/model/TransactionFeeCalculator.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/core/mocks/MockTransaction.h"

namespace catapult { namespace model {

#define TEST_CLASS NotificationPublisherTests

	namespace {
		constexpr auto Currency_Mosaic_Id = UnresolvedMosaicId(1234);

		constexpr auto Plugin_Option_Flags = static_cast<mocks::PluginOptionFlags>(
				utils::to_underlying_type(mocks::PluginOptionFlags::Custom_Buffers) |
				utils::to_underlying_type(mocks::PluginOptionFlags::Publish_Custom_Notifications));

		template<typename TEntity, typename TAssertSubFunc>
		void PublishAll(const TEntity& entity, PublicationMode mode, TAssertSubFunc assertSub) {
			// Arrange:
			mocks::MockNotificationSubscriber sub;

			auto registry = mocks::CreateDefaultTransactionRegistry(Plugin_Option_Flags);
			model::TransactionFeeCalculator transactionFeeCalculator;
			auto pPub = CreateNotificationPublisher(registry, Currency_Mosaic_Id, transactionFeeCalculator, mode);

			// Act:
			auto hash = test::GenerateRandomByteArray<Hash256>();
			pPub->publish(WeakEntityInfo(entity, hash, Height{0}), sub);

			// Assert:
			assertSub(sub);
		}

		template<typename TEntity, typename TAssertSubFunc>
		void PublishAll(const TEntity& entity, TAssertSubFunc assertSub) {
			PublishAll(entity, PublicationMode::All, assertSub);
		}

		template<typename TNotification, typename TEntity, typename TAssertSubFunc>
		void PublishAllTyped(const TEntity& entity, TAssertSubFunc assertSub) {
			// Arrange:
			mocks::MockTypedNotificationSubscriber<TNotification> sub;

			auto registry = mocks::CreateDefaultTransactionRegistry(Plugin_Option_Flags);
			model::TransactionFeeCalculator transactionFeeCalculator;
			auto pPub = CreateNotificationPublisher(registry, Currency_Mosaic_Id, transactionFeeCalculator);

			// Act:
			auto hash = test::GenerateRandomByteArray<Hash256>();
			pPub->publish(WeakEntityInfo(entity, hash, Height{0}), sub);

			// Assert:
			assertSub(sub);
		}

		template<typename TNotification, typename TAssertNotification>
		void PublishOne(const WeakEntityInfo& entityInfo, TAssertNotification assertNotification) {
			// Arrange:
			mocks::MockTypedNotificationSubscriber<TNotification> sub;

			auto registry = mocks::CreateDefaultTransactionRegistry(Plugin_Option_Flags);
			model::TransactionFeeCalculator transactionFeeCalculator;
			auto pPub = CreateNotificationPublisher(registry, Currency_Mosaic_Id, transactionFeeCalculator);

			// Act:
			pPub->publish(entityInfo, sub);

			// Assert:
			ASSERT_EQ(1u, sub.numMatchingNotifications());
			assertNotification(sub.matchingNotifications()[0]);
		}

		template<typename TNotification, typename TEntity, typename TAssertNotification>
		void PublishOne(const TEntity& entity, const Hash256& hash, TAssertNotification assertNotification) {
			// Act:
			PublishOne<TNotification>(WeakEntityInfo(entity, hash, Height{0}), assertNotification);
		}

		template<typename TNotification, typename TEntity, typename TAssertNotification>
		void PublishOne(const TEntity& entity, TAssertNotification assertNotification) {
			// Act:
			PublishOne<TNotification>(entity, test::GenerateRandomByteArray<Hash256>(), assertNotification);
		}
	}

	// region block

	TEST(TEST_CLASS, CanRaiseBlockSourceChangeNotifications) {
		// Arrange:
		auto pBlock = test::GenerateEmptyRandomBlock();

		// Act:
		PublishOne<SourceChangeNotification<1>>(*pBlock, [](const auto& notification) {
			// Assert:
			EXPECT_EQ(0u, notification.PrimaryId);
			EXPECT_EQ(0u, notification.SecondaryId);
			EXPECT_EQ(SourceChangeNotification<1>::SourceChangeType::Absolute, notification.PrimaryChangeType);
			EXPECT_EQ(SourceChangeNotification<1>::SourceChangeType::Absolute, notification.SecondaryChangeType);
		});
	}

	TEST(TEST_CLASS, CanRaiseBlockAccountNotifications_NonZeroBeneficiary) {
		// Arrange:
		auto pBlock = test::GenerateEmptyRandomBlock();
		test::FillWithRandomData(pBlock->Signer);
		test::FillWithRandomData(pBlock->Beneficiary);

		// Act:
		PublishAll(*pBlock, [&block = *pBlock](const auto& sub) {
			// Assert:
			EXPECT_EQ(7u, sub.numNotifications());
			EXPECT_EQ(0u, sub.numAddresses());
			EXPECT_EQ(2u, sub.numKeys());

			EXPECT_TRUE(sub.contains(block.Signer));
			EXPECT_TRUE(sub.contains(block.Beneficiary));
		});
	}

	TEST(TEST_CLASS, CanRaiseBlockAccountNotifications_ZeroBeneficiary) {
		// Arrange:
		auto pBlock = test::GenerateEmptyRandomBlock();
		test::FillWithRandomData(pBlock->Signer);
		pBlock->Beneficiary = Key();

		// Act:
		PublishAll(*pBlock, [&block = *pBlock](const auto& sub) {
			// Assert:
			EXPECT_EQ(6u, sub.numNotifications());
			EXPECT_EQ(0u, sub.numAddresses());
			EXPECT_EQ(1u, sub.numKeys());

			EXPECT_TRUE(sub.contains(block.Signer));
			EXPECT_FALSE(sub.contains(block.Beneficiary));
		});
	}

	TEST(TEST_CLASS, CanRaiseBlockEntityNotifications) {
		// Arrange:
		auto pBlock = test::GenerateEmptyRandomBlock();

		// Act:
		PublishOne<EntityNotification<1>>(*pBlock, [&pBlock](const auto& notification) {
			// Assert:
			EXPECT_EQ(static_cast<NetworkIdentifier>(0x90), notification.NetworkIdentifier);
			EXPECT_EQ(pBlock->Type, notification.EntityType);
			EXPECT_EQ(0x07u, notification.EntityVersion);
		});
	}

	TEST(TEST_CLASS, CanRaiseBlockSignatureNotifications_WithoutCosignatures) {
		// Arrange:
		auto pBlock = test::GenerateEmptyRandomBlock();
		test::FillWithRandomData(pBlock->Signer);
		test::FillWithRandomData(pBlock->Signature);

		// Act:
		PublishOne<SignatureNotification<1>>(*pBlock, [&block = *pBlock](const auto& notification) {
			// Assert:
			EXPECT_EQ(block.Signer, notification.Signer);
			EXPECT_EQ(block.Signature, notification.Signature);
			EXPECT_EQ(test::AsVoidPointer(&block.Version), test::AsVoidPointer(notification.Data.pData));
			EXPECT_EQ(sizeof(BlockHeaderV4) - VerifiableEntity::Header_Size, notification.Data.Size);
			EXPECT_EQ(SignatureNotification<1>::ReplayProtectionMode::Disabled, notification.DataReplayProtectionMode);
		});
	}

	namespace {
		model::UniqueEntityPtr<Block> GenerateBlockWithTransactionSizes(const std::vector<Amount>& fees) {
			test::ConstTransactions transactions;
			for (auto fee : fees) {
				auto pTransaction = test::GenerateRandomTransactionWithSize(fee.unwrap());
				pTransaction->MaxFee = Amount(10 * fee.unwrap());
				transactions.push_back(std::move(pTransaction));
			}

			auto pBlock = test::GenerateBlockWithTransactions(transactions);
			test::FillWithRandomData(pBlock->Signer);
			pBlock->FeeInterest = 1;
			pBlock->FeeInterestDenominator = 1;
			return pBlock;
		}
	}

	TEST(TEST_CLASS, CanRaiseBlockSignatureNotifications_WithCosignatures) {
		// Arrange:
		std::vector<Cosignature> cosignatures{
			{ test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Signature>() },
			{ test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Signature>() },
			{ test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Signature>() },
		};
		auto numCosignatures = cosignatures.size();
		auto pBlock = GenerateBlockWithTransactionSizes({ Amount(numCosignatures * sizeof(Cosignature)) });
		test::FillWithRandomData(pBlock->Signer);
		test::FillWithRandomData(pBlock->Signature);
		// Convert transaction into cosignatures.
		pBlock->setTransactionPayloadSize(0u);
		auto* pData = reinterpret_cast<uint8_t*>(pBlock->CosignaturesPtr());
		std::memcpy(pData, cosignatures.data(), numCosignatures * sizeof(Cosignature));

		// Act:
		PublishAllTyped<SignatureNotification<1>>(*pBlock, [&block = *pBlock, numCosignatures, &cosignatures](const auto& sub) {
			// Assert:
			ASSERT_EQ(numCosignatures + 1u, sub.numMatchingNotifications());

			for (auto i = 0u; i < numCosignatures; ++i) {
				const auto& notification = sub.matchingNotifications()[i];
				EXPECT_EQ(cosignatures[i].Signer, notification.Signer);
				EXPECT_EQ(cosignatures[i].Signature, notification.Signature);
				EXPECT_EQ(test::AsVoidPointer(&block.Version), test::AsVoidPointer(notification.Data.pData));
				EXPECT_EQ(sizeof(BlockHeaderV4) - VerifiableEntity::Header_Size, notification.Data.Size);
				EXPECT_EQ(SignatureNotification<1>::ReplayProtectionMode::Disabled, notification.DataReplayProtectionMode);
			}

			const auto& notification = sub.matchingNotifications()[numCosignatures];
			EXPECT_EQ(block.Signer, notification.Signer);
			EXPECT_EQ(block.Signature, notification.Signature);
			EXPECT_EQ(test::AsVoidPointer(&block.Version), test::AsVoidPointer(notification.Data.pData));
			EXPECT_EQ(sizeof(BlockHeaderV4) - VerifiableEntity::Header_Size, notification.Data.Size);
			EXPECT_EQ(SignatureNotification<1>::ReplayProtectionMode::Disabled, notification.DataReplayProtectionMode);
		});
	}

	TEST(TEST_CLASS, CanRaiseBlockNotifications_BlockWithoutTransactions) {
		// Arrange:
		auto pBlock = GenerateBlockWithTransactionSizes({});
		pBlock->Timestamp = Timestamp(123);
		pBlock->Difficulty = Difficulty(575);
		pBlock->FeeMultiplier = BlockFeeMultiplier(3);
		pBlock->FeeInterest = 3;
		pBlock->FeeInterestDenominator = 7;

		// Act:
		PublishOne<BlockNotification<1>>(*pBlock, [&block = *pBlock](const auto& notification) {
			// Assert:
			EXPECT_EQ(block.Signer, notification.Signer);
			EXPECT_EQ(block.Beneficiary, notification.Beneficiary);
			EXPECT_EQ(Timestamp(123), notification.Timestamp);
			EXPECT_EQ(Difficulty(575), notification.Difficulty);
			EXPECT_EQ(Amount(0), notification.TotalFee);
			EXPECT_EQ(0u, notification.NumTransactions);
			EXPECT_EQ(3, notification.FeeInterest);
			EXPECT_EQ(7, notification.FeeInterestDenominator);
		});
	}

	TEST(TEST_CLASS, CanRaiseBlockNotifications_BlockWithTransactions) {
		// Arrange:
		auto pBlock = GenerateBlockWithTransactionSizes({ Amount(211), Amount(225), Amount(217) });
		pBlock->Timestamp = Timestamp(432);
		pBlock->Difficulty = Difficulty(575);
		pBlock->FeeMultiplier = BlockFeeMultiplier(3);
		pBlock->FeeInterest = 2;
		pBlock->FeeInterestDenominator = 2;

		// Act:
		PublishOne<BlockNotification<1>>(*pBlock, [&block = *pBlock](const auto& notification) {
			// Assert:
			EXPECT_EQ(block.Signer, notification.Signer);
			EXPECT_EQ(block.Beneficiary, notification.Beneficiary);
			EXPECT_EQ(Timestamp(432), notification.Timestamp);
			EXPECT_EQ(Difficulty(575), notification.Difficulty);
			EXPECT_EQ(Amount(3 * 653), notification.TotalFee);
			EXPECT_EQ(3u, notification.NumTransactions);
			EXPECT_EQ(2, notification.FeeInterest);
			EXPECT_EQ(2, notification.FeeInterestDenominator);
		});
	}

	TEST(TEST_CLASS, CanRaiseBlockCommitteeNotifications) {
		// Arrange:
		auto numCosignatures = 3;
		auto pBlock = GenerateBlockWithTransactionSizes({ Amount(numCosignatures * sizeof(Cosignature)) });
		pBlock->setRound(10);
		pBlock->FeeInterest = 2;
		pBlock->FeeInterestDenominator = 2;
		// Convert transaction into cosignatures.
		pBlock->setTransactionPayloadSize(0u);

		// Act:
		PublishOne<BlockCommitteeNotification<4>>(*pBlock, [&block = *pBlock, numCosignatures](const auto& notification) {
			// Assert:
			EXPECT_EQ(10, notification.Round);
			EXPECT_EQ(2, notification.FeeInterest);
			EXPECT_EQ(2, notification.FeeInterestDenominator);
		});
	}

	TEST(TEST_CLASS, CanPublishBlockNotificationsWithModeBasic_v3) {
		// Arrange:
		auto pBlock = GenerateBlockWithTransactionSizes({});
		pBlock->Version = 0x11000003;
		pBlock->Size = sizeof(model::BlockHeader);

		// Act:
		PublishAll(*pBlock, PublicationMode::Basic, [](const auto& sub) {
			// Assert: no notifications were suppressed (blocks do not have custom notifications)
			ASSERT_EQ(6u, sub.numNotifications());
			EXPECT_EQ(Core_Source_Change_v1_Notification, sub.notificationTypes()[0]);
			EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[1]);
			EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[2]);
			EXPECT_EQ(Core_Entity_v1_Notification, sub.notificationTypes()[3]);
			EXPECT_EQ(Core_Block_v1_Notification, sub.notificationTypes()[4]);
			EXPECT_EQ(Core_Signature_v1_Notification, sub.notificationTypes()[5]);
		});
	}

	TEST(TEST_CLASS, CanPublishBlockNotificationsWithModeBasic) {
		// Arrange:
		auto numCosignatures = 3u;
		auto pBlock = GenerateBlockWithTransactionSizes({ Amount(numCosignatures * sizeof(Cosignature)) });
		// Convert transaction into cosignatures.
		pBlock->setTransactionPayloadSize(0u);

		// Act:
		PublishAll(*pBlock, PublicationMode::Basic, [numCosignatures](const auto& sub) {
			// Assert: no notifications were suppressed (blocks do not have custom notifications)
			ASSERT_EQ(7u + numCosignatures, sub.numNotifications());
			auto i = 0u;
			EXPECT_EQ(Core_Source_Change_v1_Notification, sub.notificationTypes()[i++]);
			EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]);
			EXPECT_EQ(Core_Block_Committee_v4_Notification, sub.notificationTypes()[i++]);
			for (auto k = 0u; k < numCosignatures; ++k)
				EXPECT_EQ(Core_Signature_v1_Notification, sub.notificationTypes()[i++]);
			EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]);
			EXPECT_EQ(Core_Entity_v1_Notification, sub.notificationTypes()[i++]);
			EXPECT_EQ(Core_Block_v1_Notification, sub.notificationTypes()[i++]);
			EXPECT_EQ(Core_Signature_v1_Notification, sub.notificationTypes()[i++]);
		});
	}

	TEST(TEST_CLASS, CanPublishBlockNotificationsWithModeCustom) {
		// Arrange:
		auto pBlock = GenerateBlockWithTransactionSizes({});

		// Act:
		PublishAll(*pBlock, PublicationMode::Custom, [](const auto& sub) {
			// Assert: all notifications were suppressed (blocks do not have custom notifications)
			ASSERT_EQ(0u, sub.numNotifications());
		});
	}

	// endregion

	// region transaction

	TEST(TEST_CLASS, CanRaiseTransactionSourceChangeNotifications) {
		// Arrange:
		auto pTransaction = mocks::CreateMockTransaction(0);

		// Act:
		PublishOne<SourceChangeNotification<1>>(*pTransaction, [](const auto& notification) {
			// Assert:
			EXPECT_EQ(1u, notification.PrimaryId);
			EXPECT_EQ(0u, notification.SecondaryId);
			EXPECT_EQ(SourceChangeNotification<1>::SourceChangeType::Relative, notification.PrimaryChangeType);
			EXPECT_EQ(SourceChangeNotification<1>::SourceChangeType::Absolute, notification.SecondaryChangeType);
		});
	}

	TEST(TEST_CLASS, CanRaiseTransactionAccountNotifications) {
		// Arrange:
		auto pTransaction = mocks::CreateMockTransaction(0);
		test::FillWithRandomData(pTransaction->Signer);
		test::FillWithRandomData(pTransaction->Recipient);

		// Act:
		PublishAll(*pTransaction, [&transaction = *pTransaction](const auto& sub) {
			// Assert: both signer (from notification publisher) and recipient (from custom publish implementation) are raised
			EXPECT_EQ(0u, sub.numAddresses());
			EXPECT_EQ(2u, sub.numKeys());

			EXPECT_TRUE(sub.contains(transaction.Signer));
			EXPECT_TRUE(sub.contains(transaction.Recipient));
		});
	}

	TEST(TEST_CLASS, CanRaiseTransactionEntityNotifications) {
		// Arrange:
		auto pTransaction = mocks::CreateMockTransaction(0);
		pTransaction->Version = 0x1100005A;

		// Act:
		PublishOne<EntityNotification<1>>(*pTransaction, [&pTransaction](const auto& notification) {
			// Assert:
			EXPECT_EQ(static_cast<NetworkIdentifier>(0x11), notification.NetworkIdentifier);
			EXPECT_EQ(pTransaction->Type, notification.EntityType);
			EXPECT_EQ(0x5Au, notification.EntityVersion);
		});
	}

	TEST(TEST_CLASS, CanRaiseTransactionSignatureNotifications) {
		// Arrange:
		auto pTransaction = mocks::CreateMockTransaction(12);
		test::FillWithRandomData(pTransaction->Signer);
		test::FillWithRandomData(pTransaction->Signature);

		// Act:
		PublishOne<SignatureNotification<1>>(*pTransaction, [&transaction = *pTransaction](const auto& notification) {
			// Assert:
			EXPECT_EQ(transaction.Signer, notification.Signer);
			EXPECT_EQ(transaction.Signature, notification.Signature);

			// - notice that mock plugin is configured with PluginOptionFlags::Custom_Buffers so dataBuffer() contains only data payload
			EXPECT_EQ(test::AsVoidPointer(&transaction + 1), test::AsVoidPointer(notification.Data.pData));
			EXPECT_EQ(12u, notification.Data.Size);
			EXPECT_EQ(SignatureNotification<1>::ReplayProtectionMode::Enabled, notification.DataReplayProtectionMode);
		});
	}

	TEST(TEST_CLASS, CanRaiseTransactionNotifications) {
		// Arrange:
		auto hash = test::GenerateRandomByteArray<Hash256>();
		auto pTransaction = mocks::CreateMockTransaction(12);
		test::FillWithRandomData(pTransaction->Signer);
		pTransaction->Deadline = Timestamp(454);

		// Act:
		PublishOne<TransactionNotification<1>>(*pTransaction, hash, [&signer = pTransaction->Signer, &hash](const auto& notification) {
			// Assert:
			EXPECT_EQ(signer, notification.Signer);
			EXPECT_EQ(hash, notification.TransactionHash);
			EXPECT_EQ(static_cast<EntityType>(mocks::MockTransaction::Entity_Type), notification.TransactionType);
			EXPECT_EQ(Timestamp(454), notification.Deadline);
		});
	}

	TEST(TEST_CLASS, CanRaiseTransactionDeadlineNotifications) {
		// Arrange:
		auto pTransaction = mocks::CreateMockTransaction(12);
		pTransaction->Deadline = Timestamp(454);

		// Act:
		PublishOne<TransactionDeadlineNotification<1>>(*pTransaction, [](const auto& notification) {
			// Assert:
			EXPECT_EQ(Timestamp(454), notification.Deadline);
			EXPECT_EQ(utils::TimeSpan::FromMilliseconds(0xEEEE'EEEE'EEEE'1234), notification.MaxLifetime); // from MockTransaction
		});
	}

	TEST(TEST_CLASS, CanRaiseTransactionFeeNotification_BlockIndependent) {
		// Arrange:
		auto pTransaction = mocks::CreateMockTransaction(12);
		pTransaction->MaxFee = Amount(765);

		// Act:
		PublishOne<TransactionFeeNotification<1>>(*pTransaction, [transactionSize = pTransaction->Size](const auto& notification) {
			// Assert: max fee is used when there is no associated block
			EXPECT_EQ(transactionSize, notification.TransactionSize);
			EXPECT_EQ(Amount(765), notification.Fee);
			EXPECT_EQ(Amount(765), notification.MaxFee);
		});
	}

	TEST(TEST_CLASS, CanRaiseTransactionFeeNotification_BlockDependent) {
		// Arrange:
		auto hash = test::GenerateRandomByteArray<Hash256>();
		auto pTransaction = test::GenerateRandomTransactionWithSize(234);
		pTransaction->Type = mocks::MockTransaction::Entity_Type;
		pTransaction->MaxFee = Amount(765);
		BlockHeaderV4 blockHeader;
		blockHeader.FeeMultiplier = BlockFeeMultiplier(4);
		blockHeader.FeeInterest = 1;
		blockHeader.FeeInterestDenominator = 1;

		// Act:
		PublishOne<TransactionFeeNotification<1>>(WeakEntityInfo(*pTransaction, hash, blockHeader), [transactionSize = pTransaction->Size](
				const auto& notification) {
			// Assert: calculated fee is used when there is associated block
			EXPECT_EQ(transactionSize, notification.TransactionSize);
			EXPECT_EQ(Amount(4 * 234), notification.Fee);
			EXPECT_EQ(Amount(765), notification.MaxFee);
		});
	}

	TEST(TEST_CLASS, CanRaiseTransactionFeeDebitNotifications) {
		// Arrange:
		auto pTransaction = mocks::CreateMockTransaction(12);
		test::FillWithRandomData(pTransaction->Signer);
		pTransaction->MaxFee = Amount(765);

		// Act:
		PublishOne<BalanceDebitNotification<1>>(*pTransaction, [&signer = pTransaction->Signer](const auto& notification) {
			// Assert:
			EXPECT_EQ(signer, notification.Sender);
			EXPECT_EQ(Currency_Mosaic_Id, notification.MosaicId);
			EXPECT_EQ(Amount(765), notification.Amount);
		});
	}

	TEST(TEST_CLASS, CanRaiseCustomTransactionNotifications) {
		// Arrange:
		auto pTransaction = mocks::CreateMockTransaction(12);

		// Act:
		PublishAll(*pTransaction, [&transaction = *pTransaction](const auto& sub) {
			// Assert: 8 raised by NotificationPublisher, 8 raised by MockTransaction::publish (first is AccountPublicKeyNotification)
			ASSERT_EQ(8u + 1 + 7, sub.numNotifications());

			size_t startIndex = 9;
			EXPECT_EQ(mocks::Mock_Observer_1_Notification, sub.notificationTypes()[startIndex]);
			EXPECT_EQ(mocks::Mock_Validator_1_Notification, sub.notificationTypes()[startIndex + 1]);
			EXPECT_EQ(mocks::Mock_All_1_Notification, sub.notificationTypes()[startIndex + 2]);
			EXPECT_EQ(mocks::Mock_Observer_2_Notification, sub.notificationTypes()[startIndex + 3]);
			EXPECT_EQ(mocks::Mock_Validator_2_Notification, sub.notificationTypes()[startIndex + 4]);
			EXPECT_EQ(mocks::Mock_All_2_Notification, sub.notificationTypes()[startIndex + 5]);
			EXPECT_EQ(mocks::Mock_Hash_Notification, sub.notificationTypes()[startIndex + 6]);
		});
	}

	TEST(TEST_CLASS, CanRaiseCustomTransactionNotificationsDependentOnHash) {
		// Arrange:
		auto hash = test::GenerateRandomByteArray<Hash256>();
		auto pTransaction = mocks::CreateMockTransaction(12);

		// Act:
		PublishOne<mocks::HashNotification>(*pTransaction, hash, [&hash](const auto& notification) {
			// Assert:
			EXPECT_EQ(&hash, &notification.Hash);
		});
	}

	TEST(TEST_CLASS, CanPublishTransactionNotificationsWithModeBasic) {
		// Arrange:
		auto pTransaction = mocks::CreateMockTransaction(12);

		// Act:
		PublishAll(*pTransaction, PublicationMode::Basic, [&transaction = *pTransaction](const auto& sub) {
			// Assert: 8 raised by NotificationPublisher, none raised by MockTransaction::publish
			ASSERT_EQ(8u, sub.numNotifications());
			EXPECT_EQ(Core_Source_Change_v1_Notification, sub.notificationTypes()[0]);
			EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[1]);
			EXPECT_EQ(Core_Entity_v1_Notification, sub.notificationTypes()[2]);
			EXPECT_EQ(Core_Transaction_v1_Notification, sub.notificationTypes()[3]);
			EXPECT_EQ(Core_Transaction_Deadline_v1_Notification, sub.notificationTypes()[4]);
			EXPECT_EQ(Core_Transaction_Fee_v1_Notification, sub.notificationTypes()[5]);
			EXPECT_EQ(Core_Balance_Debit_v1_Notification, sub.notificationTypes()[6]);
			EXPECT_EQ(Core_Signature_v1_Notification, sub.notificationTypes()[7]);
		});
	}

	TEST(TEST_CLASS, CanPublishTransactionNotificationsWithModeCustom) {
		// Arrange:
		auto pTransaction = mocks::CreateMockTransaction(12);

		// Act:
		PublishAll(*pTransaction, PublicationMode::Custom, [&transaction = *pTransaction](const auto& sub) {
			// Assert: 8 raised by MockTransaction::publish (first is AccountPublicKeyNotification)
			ASSERT_EQ(1u + 7, sub.numNotifications());

			EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[0]);
			EXPECT_EQ(mocks::Mock_Observer_1_Notification, sub.notificationTypes()[1]);
			EXPECT_EQ(mocks::Mock_Validator_1_Notification, sub.notificationTypes()[2]);
			EXPECT_EQ(mocks::Mock_All_1_Notification, sub.notificationTypes()[3]);
			EXPECT_EQ(mocks::Mock_Observer_2_Notification, sub.notificationTypes()[4]);
			EXPECT_EQ(mocks::Mock_Validator_2_Notification, sub.notificationTypes()[5]);
			EXPECT_EQ(mocks::Mock_All_2_Notification, sub.notificationTypes()[6]);
			EXPECT_EQ(mocks::Mock_Hash_Notification, sub.notificationTypes()[7]);
		});
	}

	// endregion

	// region other

	TEST(TEST_CLASS, CannotRaiseAnyNotificationsForUnknownEntities) {
		// Arrange:
		VerifiableEntity entity{};

		// Act:
		EXPECT_THROW(PublishOne<SourceChangeNotification<1>>(entity, [](const auto&) {}), catapult_runtime_error);
	}

	TEST(TEST_CLASS, CannotRaiseAnyNotificationsForUnknownEntitiesWithModeBasic) {
		// Arrange:
		VerifiableEntity entity{};

		// Act:
		EXPECT_THROW(PublishAll(entity, PublicationMode::Basic, [](const auto&) {}), catapult_runtime_error);
	}

	TEST(TEST_CLASS, CannotRaiseAnyNotificationsForUnknownEntitiesWithModeCustom) {
		// Arrange:
		VerifiableEntity entity{};

		// Act:
		EXPECT_THROW(PublishAll(entity, PublicationMode::Custom, [](const auto&) {}), catapult_runtime_error);
	}

	// endregion
}}
