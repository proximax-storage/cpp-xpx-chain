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

#include "partialtransaction/src/chain/AggregateCosignersNotificationPublisher.h"
#include "plugins/txes/aggregate/src/model/AggregateNotifications.h"
#include "plugins/txes/aggregate/tests/test/AggregateTestUtils.h"
#include "catapult/model/WeakCosignedTransactionInfo.h"
#include "partialtransaction/tests/test/AggregateTransactionTestUtils.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/TestHarness.h"

using namespace catapult::model;

namespace catapult { namespace chain {

#define TEST_CLASS AggregateCosignersNotificationPublisherTests

	namespace {
		struct V1TestTraits {
			using Descriptor = model::AggregateTransactionRawDescriptor;
			using EmbeddedNotification = model::AggregateEmbeddedTransactionNotification<1>;
			using CosignaturesNotification = model::AggregateCosignaturesNotification<1>;
			static inline const std::vector<model::EntityType> EntityTypes = { Entity_Type_Aggregate_Complete_V1, Entity_Type_Aggregate_Bonded_V1 };
		};

		struct V2TestTraits {
			using Descriptor = model::AggregateTransactionExtendedDescriptor;
			using EmbeddedNotification = model::AggregateEmbeddedTransactionNotification<2>;
			using CosignaturesNotification = model::AggregateCosignaturesNotification<3>;
			static inline const std::vector<model::EntityType> EntityTypes = { Entity_Type_Aggregate_Complete_V2, Entity_Type_Aggregate_Bonded_V2 };
		};
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_V1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V1TestTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_V2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V2TestTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()
	// region basic

	TRAITS_BASED_TEST(NonAggregateTransactionIsNotSupported) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<typename TTraits::EmbeddedNotification> sub;
		AggregateCosignersNotificationPublisher publisher;
		auto wrapper = test::CreateAggregateTransaction<typename TTraits::Descriptor>(2);

		auto cosignatures = test::GenerateRandomDataVector<CosignatureInfo>(4);
		auto transactionInfo = WeakCosignedTransactionInfo(wrapper.pTransaction.get(), &cosignatures);

		// - change the transaction type
		DEFINE_TRANSACTION_TYPE(Aggregate, Non_Aggregate_Type, 0xFF);
		wrapper.pTransaction->Type = Entity_Type_Non_Aggregate_Type;

		// Act + Assert: transfer type is not supported
		EXPECT_THROW(publisher.publish(transactionInfo, sub), catapult_invalid_argument);
	}

	TRAITS_BASED_TEST(AggregateWithCosignaturesIsNotSupported) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<typename TTraits::EmbeddedNotification> sub;
		AggregateCosignersNotificationPublisher publisher;
		auto wrapper = test::CreateAggregateTransaction<typename TTraits::Descriptor>(2);

		auto cosignatures = test::GenerateRandomDataVector<CosignatureInfo>(4);
		auto transactionInfo = WeakCosignedTransactionInfo(wrapper.pTransaction.get(), &cosignatures);

		// - make the transaction look like it has a cosignature
		//   (since the transactions are not iterated before the check, only the first transaction needs to be valid)
		wrapper.pTransaction->PayloadSize -= sizeof(typename TTraits::Descriptor::CosignatureType);

		// Act + Assert: aggregate must not have any cosignatures
		EXPECT_THROW(publisher.publish(transactionInfo, sub), catapult_invalid_argument);
	}

	// endregion

	// region publish: aggregate embedded transaction

	namespace {
		template<typename TTraits>
		void AssertCanRaiseEmbeddedTransactionNotifications(
				uint8_t numTransactions,
				uint8_t numCosignatures,
				model::EntityType transactionType) {
			// Arrange:
			mocks::MockTypedNotificationSubscriber<typename TTraits::EmbeddedNotification> sub;
			AggregateCosignersNotificationPublisher publisher;
			auto wrapper = test::CreateAggregateTransaction<typename TTraits::Descriptor>(numTransactions);
			wrapper.pTransaction->Type = transactionType;

			auto cosignatures = test::GenerateRandomDataVector<CosignatureInfo>(numCosignatures);
			auto transactionInfo = WeakCosignedTransactionInfo(wrapper.pTransaction.get(), &cosignatures);

			// Act:
			publisher.publish(transactionInfo, sub);

			// Assert: the plugin raises an embedded transaction notification for each transaction
			ASSERT_EQ(numTransactions, sub.numMatchingNotifications());
			for (auto i = 0u; i < numTransactions; ++i) {
				std::ostringstream out;
				out << "transaction at " << i << " (" << transactionType << ")";
				auto message = out.str();
				const auto& notification = sub.matchingNotifications()[i];

				EXPECT_EQ(wrapper.pTransaction->Signer, notification.Signer) << message;
				EXPECT_EQ(*wrapper.SubTransactions[i], notification.Transaction) << message;
				EXPECT_EQ(numCosignatures, notification.CosignaturesCount);
				test::CompareCosignatures(cosignatures.data(), notification.CosignaturesPtr, numCosignatures);

			}
		}
	}

	TRAITS_BASED_TEST(EmptyAggregateDoesNotRaiseEmbeddedTransactionNotifications) {
		// Assert:
		for (auto transactionType : TTraits::EntityTypes)
			AssertCanRaiseEmbeddedTransactionNotifications<TTraits>(0, 0, transactionType);
	}

	TRAITS_BASED_TEST(CanRaiseEmbeddedTransactionNotificationsFromAggregate) {
		// Assert:
		for (auto transactionType : TTraits::EntityTypes)
			AssertCanRaiseEmbeddedTransactionNotifications<TTraits>(2, 3, transactionType);
	}


	// endregion

	// region publish: aggregate cosignatures

	TRAITS_BASED_TEST(CanRaiseAggregateCosignaturesNotificationsFromEmptyAggregate) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<typename TTraits::CosignaturesNotification> sub;
		AggregateCosignersNotificationPublisher publisher;
		auto wrapper = test::CreateAggregateTransaction<typename TTraits::Descriptor>(0);

		auto cosignatures = test::GenerateRandomDataVector<CosignatureInfo>(0);
		auto transactionInfo = WeakCosignedTransactionInfo(wrapper.pTransaction.get(), &cosignatures);

		// Act:
		publisher.publish(transactionInfo, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(wrapper.pTransaction->Signer, notification.Signer);
		EXPECT_EQ(0u, notification.TransactionsCount);
		EXPECT_FALSE(!!notification.TransactionsPtr);
		EXPECT_EQ(0u, notification.CosignaturesCount);
		EXPECT_FALSE(!!notification.CosignaturesPtr);
	}

	TRAITS_BASED_TEST(CanRaiseAggregateCosignaturesNotificationsFromAggregate) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<typename TTraits::CosignaturesNotification> sub;
		AggregateCosignersNotificationPublisher publisher;
		auto wrapper = test::CreateAggregateTransaction<typename TTraits::Descriptor>(2);

		auto cosignatures = test::GenerateRandomDataVector<CosignatureInfo>(3);
		auto transactionInfo = WeakCosignedTransactionInfo(wrapper.pTransaction.get(), &cosignatures);

		// Act:
		publisher.publish(transactionInfo, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(wrapper.pTransaction->Signer, notification.Signer);
		EXPECT_EQ(2u, notification.TransactionsCount);
		EXPECT_EQ(wrapper.pTransaction->TransactionsPtr(), notification.TransactionsPtr);
		EXPECT_EQ(3u, notification.CosignaturesCount);
		EXPECT_EQ(cosignatures.data()->GetRawSignature(), notification.CosignaturesPtr.GetRawSignature());
		EXPECT_EQ(cosignatures.data()->Signer, notification.CosignaturesPtr.Signer());
	}

	// endregion
}}
