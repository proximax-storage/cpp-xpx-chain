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

#include "catapult/consumers/TransactionConsumers.h"
#include "tests/catapult/consumers/test/ConsumerInputFactory.h"
#include "tests/catapult/consumers/test/ConsumerTestUtils.h"
#include "tests/test/nodeps/ParamsCapture.h"

using catapult::disruptor::ConsumerInput;

namespace catapult { namespace consumers {

#define TEST_CLASS NewTransactionsConsumerTests

	namespace {
		struct NewTransactionsSinkParams {
		public:
			explicit NewTransactionsSinkParams(std::vector<model::TransactionInfo>&& addedTransactionInfos)
					: AddedTransactionInfos(CopyInfos(addedTransactionInfos))
			{}

		private:
			static std::vector<model::TransactionInfo> CopyInfos(const std::vector<model::TransactionInfo>& transactionInfos) {
				std::vector<model::TransactionInfo> copy;
				copy.reserve(transactionInfos.size());
				for (const auto& transactionInfo : transactionInfos)
					copy.emplace_back(transactionInfo.copy());

				return copy;
			}

		public:
			std::vector<model::TransactionInfo> AddedTransactionInfos;
		};

		class MockNewTransactionsSink : public test::ParamsCapture<NewTransactionsSinkParams> {
		public:
			void operator()(std::vector<model::TransactionInfo>&& addedTransactionInfos) const {
				const_cast<MockNewTransactionsSink*>(this)->push(std::move(addedTransactionInfos));
			}
		};

		struct ConsumerTestContext {
		public:
			ConsumerTestContext()
					: Consumer(CreateNewTransactionsConsumer([&handler = NewTransactionsSink](auto&& transactionInfos) {
						handler(std::move(transactionInfos));
					}))
			{}

		public:
			MockNewTransactionsSink NewTransactionsSink;
			disruptor::DisruptorConsumer Consumer;
		};

		ConsumerInput CreateInput(size_t numTransactions) {
			auto input = test::CreateConsumerInputWithTransactions(numTransactions, disruptor::InputSource::Unknown);
			for (auto& element : input.transactions())
				element.OptionalExtractedAddresses = std::make_shared<model::UnresolvedAddressSet>();

			return input;
		}

		void AssertEqual(
				const disruptor::FreeTransactionElement& element,
				const model::TransactionInfo& transactionInfo,
				const std::string& message) {
			// Assert:
			EXPECT_EQ(&element.Transaction, transactionInfo.pEntity.get()) << message;
			EXPECT_EQ(element.EntityHash, transactionInfo.EntityHash) << message;
			EXPECT_EQ(element.MerkleComponentHash, transactionInfo.MerkleComponentHash) << message;

			EXPECT_TRUE(!!element.OptionalExtractedAddresses) << message;
			EXPECT_EQ(element.OptionalExtractedAddresses.get(), transactionInfo.OptionalExtractedAddresses.get()) << message;

			// Sanity:
			EXPECT_EQ(disruptor::ConsumerResultSeverity::Success, element.ResultSeverity) << message;
		}
	}

	TEST(TEST_CLASS, CanProcessZeroEntities) {
		// Assert:
		ConsumerTestContext context;
		test::AssertPassthroughForEmptyInput(context.Consumer);
	}

	TEST(TEST_CLASS, AllEntitiesAreForwardedWhenNoneAreSkipped) {
		// Arrange:
		ConsumerTestContext context;
		auto input = CreateInput(3);

		// Act:
		auto result = context.Consumer(input);

		// Assert: the consumer detached the input
		test::AssertConsumed(result, validators::ValidationResult::Success);
		EXPECT_TRUE(input.empty());

		// - the new transactions handler was called once (with three entities)
		const auto& params = context.NewTransactionsSink.params();
		ASSERT_EQ(1u, params.size());
		const auto& actualInfos = params[0].AddedTransactionInfos;
		ASSERT_EQ(3u, actualInfos.size());

		auto i = 0u;
		for (const auto& info : actualInfos) {
			AssertEqual(input.transactions()[i], info, "info at " + std::to_string(i));
			++i;
		}
	}

	TEST(TEST_CLASS, NoEntitiesAreForwardedWhenAllAreSkipped) {
		// Arrange:
		ConsumerTestContext context;
		auto input = CreateInput(3);
		auto i = 0u;
		for (auto& element : input.transactions()) {
			element.ResultSeverity = 0 == i % 2 ? disruptor::ConsumerResultSeverity::Failure : disruptor::ConsumerResultSeverity::Neutral;
			++i;
		}

		// Act:
		auto result = context.Consumer(input);

		// Assert: the consumer detached the input
		test::AssertAborted(result, validators::ValidationResult::Failure);
		EXPECT_TRUE(input.empty());

		// - the new transactions handler was not called
		const auto& params = context.NewTransactionsSink.params();
		ASSERT_EQ(0u, params.size());
	}

	TEST(TEST_CLASS, OnlyNonSkippedElementsAreForwardedWhenSomeAreSkipped) {
		// Arrange:
		ConsumerTestContext context;
		auto input = CreateInput(5);
		input.transactions()[0].ResultSeverity = disruptor::ConsumerResultSeverity::Failure;
		input.transactions()[2].ResultSeverity = disruptor::ConsumerResultSeverity::Neutral;
		input.transactions()[3].ResultSeverity = disruptor::ConsumerResultSeverity::Failure;

		// Act:
		auto result = context.Consumer(input);

		// Assert: the consumer detached the input
		test::AssertAborted(result, validators::ValidationResult::Failure);
		EXPECT_TRUE(input.empty());

		// - the new transactions handler was called once (with two entities)
		const auto& params = context.NewTransactionsSink.params();
		ASSERT_EQ(1u, params.size());
		const auto& actualInfos = params[0].AddedTransactionInfos;
		ASSERT_EQ(2u, actualInfos.size());

		AssertEqual(input.transactions()[1], actualInfos[0], "info at 0");
		AssertEqual(input.transactions()[4], actualInfos[1], "info at 1");
	}

	namespace {
		void AssertAggregateResult(
				validators::ValidationResult expectedAggregateResult,
				const std::vector<disruptor::ConsumerResultSeverity>& results) {
			// Arrange:
			ConsumerTestContext context;
			auto input = CreateInput(results.size());
			for (auto i = 0u; i < results.size(); ++i)
				input.transactions()[i].ResultSeverity = results[i];

			// Act:
			auto result = context.Consumer(input);

			// Assert:
			if (validators::ResultSeverity::Failure != validators::GetSeverity(expectedAggregateResult))
				test::AssertConsumed(result, expectedAggregateResult);
			else
				test::AssertAborted(result, expectedAggregateResult);
		}
	}

	TEST(TEST_CLASS, SuccessResultWhenAtLeastOneResultIsSuccessAndNoResultIsFailure) {
		// Assert:
		AssertAggregateResult(validators::ValidationResult::Success, {
			disruptor::ConsumerResultSeverity::Success,
			disruptor::ConsumerResultSeverity::Success,
			disruptor::ConsumerResultSeverity::Success
		});
		AssertAggregateResult(validators::ValidationResult::Success, {
			disruptor::ConsumerResultSeverity::Success,
			disruptor::ConsumerResultSeverity::Neutral,
			disruptor::ConsumerResultSeverity::Success,
			disruptor::ConsumerResultSeverity::Success,
			disruptor::ConsumerResultSeverity::Neutral
		});
	}

	TEST(TEST_CLASS, NeutralResultWhenAllResultsAreNeutral) {
		// Assert:
		AssertAggregateResult(validators::ValidationResult::Neutral, {
			disruptor::ConsumerResultSeverity::Neutral,
			disruptor::ConsumerResultSeverity::Neutral,
			disruptor::ConsumerResultSeverity::Neutral
		});
	}

	TEST(TEST_CLASS, FailureResultWhenAtLeastOneResultIsFailure) {
		// Assert:
		AssertAggregateResult(validators::ValidationResult::Failure, {
			disruptor::ConsumerResultSeverity::Success,
			disruptor::ConsumerResultSeverity::Failure,
			disruptor::ConsumerResultSeverity::Success
		});
		AssertAggregateResult(validators::ValidationResult::Failure, {
			disruptor::ConsumerResultSeverity::Neutral,
			disruptor::ConsumerResultSeverity::Failure,
			disruptor::ConsumerResultSeverity::Success
		});
		AssertAggregateResult(validators::ValidationResult::Failure, {
			disruptor::ConsumerResultSeverity::Neutral,
			disruptor::ConsumerResultSeverity::Failure,
			disruptor::ConsumerResultSeverity::Neutral
		});
		AssertAggregateResult(validators::ValidationResult::Failure, {
			disruptor::ConsumerResultSeverity::Failure,
			disruptor::ConsumerResultSeverity::Failure,
			disruptor::ConsumerResultSeverity::Failure
		});
	}
}}
