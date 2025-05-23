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

#include "zeromq/src/ZeroMqUtChangeSubscriber.h"
#include "zeromq/tests/test/ZeroMqTransactionsChangeTestUtils.h"

namespace catapult { namespace zeromq {

#define TEST_CLASS ZeroMqUtChangeSubscriberTests

	namespace {
		class MqSubscriberContext : public test::MqContextT<cache::UtChangeSubscriber> {
		public:
			using MqContext::subscribeAll;

		public:
			MqSubscriberContext() : MqContextT(CreateZeroMqUtChangeSubscriber)
			{}

		public:
			void notifyAdd(const model::TransactionInfo& transactionInfo) {
				cache::UtChangeSubscriber::TransactionInfos transactionInfos;
				transactionInfos.emplace(transactionInfo.copy());
				notifyAdds(transactionInfos);
			}

			void notifyAdds(const cache::UtChangeSubscriber::TransactionInfos& transactionInfos) {
				subscriber().notifyAdds(transactionInfos);
			}

			void notifyRemove(const model::TransactionInfo& transactionInfo) {
				cache::UtChangeSubscriber::TransactionInfos transactionInfos;
				transactionInfos.emplace(transactionInfo.copy());
				notifyRemoves(transactionInfos);
			}

			void notifyRemoves(const cache::UtChangeSubscriber::TransactionInfos& transactionInfos) {
				subscriber().notifyRemoves(transactionInfos);
			}

			void flush() {
				subscriber().flush();
			}

		public:
			template<typename TTransactionInfos>
			void subscribeAll(TransactionMarker topicMarker, const TTransactionInfos& transactionInfos) {
				for (const auto& transactionInfo : transactionInfos) {
					auto addresses = test::ExtractAddresses(test::ToMockTransaction(*transactionInfo.pEntity));
					test::SubscribeForAddresses(zmqSocket(), topicMarker, addresses);
				}

				waitForReceiveSuccess();
			}
		};
	}

	// region basic tests

	TEST(TEST_CLASS, SubscriberDoesNotReceiveDataOnDifferentTopic) {
		// Arrange:
		uint64_t topic(0x12345678);
		MqSubscriberContext context;
		context.subscribe(topic);
		auto transactionInfo = test::CreateRandomTransactionInfo();

		// Act:
		context.notifyAdd(transactionInfo);

		// Assert:
		test::AssertNoPendingMessages(context.zmqSocket());
	}

	// endregion

	namespace {
		constexpr size_t Num_Transactions = 5;
		constexpr auto Add_Marker = TransactionMarker::Unconfirmed_Transaction_Add_Marker;
		constexpr auto Remove_Marker = TransactionMarker::Unconfirmed_Transaction_Remove_Marker;
	}

	// region notifyAdd

	TEST(TEST_CLASS, CanAddSingleTransaction) {
		// Assert:
		test::AssertCanAddSingleTransaction<MqSubscriberContext>(Add_Marker, [](auto& context, const auto& transactionInfo) {
			context.notifyAdd(transactionInfo);
		});
	}

	TEST(TEST_CLASS, CanAddMultipleTransactions_SingleCall) {
		// Assert:
		test::AssertCanAddMultipleTransactions<MqSubscriberContext>(
				Add_Marker,
				Num_Transactions,
				[](auto& context, const auto& transactionInfos) {
					context.notifyAdds(transactionInfos);
				});
	}

	TEST(TEST_CLASS, CanAddMultipleTransactions_MultipleCalls) {
		// Assert:
		test::AssertCanAddMultipleTransactions<MqSubscriberContext>(
				Add_Marker,
				Num_Transactions,
				[](auto& context, const auto& transactionInfos) {
					for (const auto& transactionInfo : transactionInfos)
						context.notifyAdd(transactionInfo);
				});
	}

	// endregion

	// region notifyRemove

	TEST(TEST_CLASS, CanRemoveSingleTransaction) {
		// Assert:
		test::AssertCanRemoveSingleTransaction<MqSubscriberContext>(Remove_Marker, [](auto& context, const auto& transactionInfo) {
			context.notifyRemove(transactionInfo);
		});
	}

	TEST(TEST_CLASS, CanRemoveMultipleTransactions_SingleCall) {
		// Assert:
		test::AssertCanRemoveMultipleTransactions<MqSubscriberContext>(
				Remove_Marker,
				Num_Transactions,
				[](auto& context, const auto& transactionInfos) {
					context.notifyRemoves(transactionInfos);
				});
	}

	TEST(TEST_CLASS, CanRemoveMultipleTransactions_MultipleCalls) {
		// Assert:
		test::AssertCanRemoveMultipleTransactions<MqSubscriberContext>(
				Remove_Marker,
				Num_Transactions,
				[](auto& context, const auto& transactionInfos) {
					for (const auto& transactionInfo : transactionInfos)
						context.notifyRemove(transactionInfo);
				});
	}

	// endregion

	// region flush

	TEST(TEST_CLASS, FlushDoesNotSendMessages) {
		// Assert:
		test::AssertFlushDoesNotSendMessages<MqSubscriberContext>();
	}

	// endregion
}}
