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

#include "zeromq/src/ZeroMqPtChangeSubscriber.h"
#include "zeromq/tests/test/ZeroMqTransactionsChangeTestUtils.h"

namespace catapult { namespace zeromq {

#define TEST_CLASS ZeroMqPtChangeSubscriberTests

	namespace {
		class MqSubscriberContext : public test::MqContextT<cache::PtChangeSubscriber> {
		public:
			using MqContext::subscribeAll;

		public:
			MqSubscriberContext() : MqContextT(CreateZeroMqPtChangeSubscriber)
			{}

		public:
			void notifyAddPartial(const model::TransactionInfo& transactionInfo) {
				cache::PtChangeSubscriber::TransactionInfos transactionInfos;
				transactionInfos.emplace(transactionInfo.copy());
				notifyAddPartials(transactionInfos);
			}

			void notifyAddPartials(const cache::PtChangeSubscriber::TransactionInfos& transactionInfos) {
				subscriber().notifyAddPartials(transactionInfos);
			}

			void notifyAddCosignature(const model::TransactionInfo& parentTransactionInfo, const Key& signer, const Signature& signature) {
				subscriber().notifyAddCosignature(parentTransactionInfo, signer, signature);
			}

			void notifyRemovePartial(const model::TransactionInfo& transactionInfo) {
				cache::PtChangeSubscriber::TransactionInfos transactionInfos;
				transactionInfos.emplace(transactionInfo.copy());
				notifyRemovePartials(transactionInfos);
			}

			void notifyRemovePartials(const cache::PtChangeSubscriber::TransactionInfos& transactionInfos) {
				subscriber().notifyRemovePartials(transactionInfos);
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
		context.notifyAddPartial(transactionInfo);

		// Assert:
		test::AssertNoPendingMessages(context.zmqSocket());
	}

	// endregion

	namespace {
		constexpr size_t Num_Transactions = 5;
		constexpr auto Add_Marker = TransactionMarker::Partial_Transaction_Add_Marker;
		constexpr auto Remove_Marker = TransactionMarker::Partial_Transaction_Remove_Marker;
	}

	// region notifyAddPartial

	TEST(TEST_CLASS, CanAddSinglePartialTransaction) {
		// Assert:
		test::AssertCanAddSingleTransaction<MqSubscriberContext>(Add_Marker, [](auto& context, const auto& transactionInfo) {
			context.notifyAddPartial(transactionInfo);
		});
	}

	TEST(TEST_CLASS, CanAddMultiplePartialTransactions_SingleCall) {
		// Assert:
		test::AssertCanAddMultipleTransactions<MqSubscriberContext>(
				Add_Marker,
				Num_Transactions,
				[](auto& context, const auto& transactionInfos) {
					context.notifyAddPartials(transactionInfos);
				});
	}

	TEST(TEST_CLASS, CanAddMultiplePartialTransactions_MultipleCalls) {
		// Assert:
		test::AssertCanAddMultipleTransactions<MqSubscriberContext>(
				Add_Marker,
				Num_Transactions,
				[](auto& context, const auto& transactionInfos) {
					for (const auto& transactionInfo : transactionInfos)
						context.notifyAddPartial(transactionInfo);
				});
	}

	// endregion

	// region notifyAddCosignature

	TEST(TEST_CLASS, CanAddSingleCosignature) {
		// Arrange:
		MqSubscriberContext context;
		auto marker = TransactionMarker::Cosignature_Marker;
		auto transactionInfo = test::RemoveExtractedAddresses(test::CreateRandomTransactionInfo());
		auto signer = test::GenerateRandomByteArray<Key>();
		auto signature = test::GenerateRandomByteArray<Signature>();
		auto addresses = test::ExtractAddresses(test::ToMockTransaction(*transactionInfo.pEntity));
		context.subscribeAll(marker, addresses);

		// Act:
		context.notifyAddCosignature(transactionInfo, signer, signature);

		// Assert:
		model::DetachedCosignature detachedCosignature(signer, signature, transactionInfo.EntityHash);
		test::AssertMessages(context.zmqSocket(), marker, addresses, [&detachedCosignature](const auto& message, const auto& topic) {
			test::AssertDetachedCosignatureMessage(message, topic, detachedCosignature);
		});

		test::AssertNoPendingMessages(context.zmqSocket());
	}

	TEST(TEST_CLASS, CanAddMultipleCosignatures) {
		// Arrange:
		MqSubscriberContext context;
		auto marker = TransactionMarker::Cosignature_Marker;
		auto transactionInfos = test::RemoveExtractedAddresses(test::CreateTransactionInfos(Num_Transactions));
		auto signers = test::GenerateRandomDataVector<Key>(Num_Transactions);
		auto signatures = test::GenerateRandomDataVector<Signature>(Num_Transactions);
		context.subscribeAll(TransactionMarker::Cosignature_Marker, transactionInfos);

		// Act:
		auto i = 0u;
		for (const auto& transactionInfo : transactionInfos) {
			context.notifyAddCosignature(transactionInfo, signers[i], signatures[i]);
			++i;
		}

		// Assert:
		i = 0u;
		for (const auto& transactionInfo : transactionInfos) {
			auto addresses = test::ExtractAddresses(test::ToMockTransaction(*transactionInfo.pEntity));
			model::DetachedCosignature detachedCosignature(signers[i], signatures[i], transactionInfo.EntityHash);
			test::AssertMessages(context.zmqSocket(), marker, addresses, [&detachedCosignature](const auto& message, const auto& topic) {
				test::AssertDetachedCosignatureMessage(message, topic, detachedCosignature);
			});

			++i;
		}

		test::AssertNoPendingMessages(context.zmqSocket());
	}

	// endregion

	// region notifyRemovePartial

	TEST(TEST_CLASS, CanRemoveSinglePartialTransaction) {
		// Assert:
		test::AssertCanRemoveSingleTransaction<MqSubscriberContext>(Remove_Marker, [](auto& context, const auto& transactionInfo) {
			context.notifyRemovePartial(transactionInfo);
		});
	}

	TEST(TEST_CLASS, CanRemoveMultiplePartialTransactions_SingleCall) {
		// Assert:
		test::AssertCanRemoveMultipleTransactions<MqSubscriberContext>(
				Remove_Marker,
				Num_Transactions,
				[](auto& context, const auto& transactionInfos) {
					context.notifyRemovePartials(transactionInfos);
				});
	}

	TEST(TEST_CLASS, CanRemoveMultiplePartialTransactions_MultipleCalls) {
		// Assert:
		test::AssertCanRemoveMultipleTransactions<MqSubscriberContext>(
				Remove_Marker,
				Num_Transactions,
				[](auto& context, const auto& transactionInfos) {
					for (const auto& transactionInfo : transactionInfos)
						context.notifyRemovePartial(transactionInfo);
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
