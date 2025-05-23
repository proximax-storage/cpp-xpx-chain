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

#include "partialtransaction/src/PtDispatcherService.h"
#include "partialtransaction/src/PtBootstrapperService.h"
#include "plugins/txes/aggregate/src/model/AggregateNotifications.h"
#include "plugins/txes/aggregate/src/validators/Results.h"
#include "catapult/cache_tx/MemoryPtCache.h"
#include "catapult/consumers/ConsumerResults.h"
#include "catapult/model/TransactionFeeCalculator.h"
#include "catapult/disruptor/ConsumerDispatcher.h"
#include "catapult/ionet/BroadcastUtils.h"
#include "catapult/model/EntityHasher.h"
#include "partialtransaction/tests/test/AggregateTransactionTestUtils.h"
#include "tests/test/core/PacketPayloadTestUtils.h"
#include "tests/test/local/ServiceLocatorTestContext.h"
#include "tests/test/local/ServiceTestUtils.h"
#include "tests/test/net/mocks/MockPacketWriters.h"
#include "tests/test/nodeps/Nemesis.h"

namespace catapult { namespace partialtransaction {

#define TEST_CLASS PtDispatcherServiceTests

	namespace {
		using ValidationResult = validators::ValidationResult;

		constexpr auto Num_Pre_Existing_Services = 3u;
		constexpr auto Num_Expected_Services = 2u + Num_Pre_Existing_Services;
		constexpr auto Num_Expected_Counters = 2u;
		constexpr auto Num_Expected_Tasks = 1u;

		constexpr auto Service_Name = "api.partial";
		constexpr auto Counter_Name = "PT ELEM TOT";
		constexpr auto Active_Counter_Name = "PT ELEM ACT";
		constexpr auto Sentinel_Counter_Value = extensions::ServiceLocator::Sentinel_Counter_Value;

		constexpr auto Num_Cosignatures = 5u;
		constexpr auto Transaction_Type = model::Entity_Type_Aggregate_Bonded;

		struct PtDispatcherServiceTraits {
			static constexpr auto CreateRegistrar = CreatePtDispatcherServiceRegistrar;
		};

		template<typename TAggregateNotification>
		bool HasAllCosignatures(const TAggregateNotification& aggregateNotification) {
			return Num_Cosignatures == aggregateNotification.CosignaturesCount;
		}

		template<typename TNotification>
		class BaseMockStatelessNotificationValidator : public validators::stateless::NotificationValidatorT<TNotification> {
		public:
			explicit BaseMockStatelessNotificationValidator(validators::ValidationResult result)
					: m_result(result)
			{}

		public:
			const std::string& name() const override {
				return m_name;
			}

			ValidationResult validate(const TNotification&) const override {
				return m_result;
			}

		protected:
			const std::string m_name = "MockStatelessNotificationValidator";
			const ValidationResult m_result;
		};

		template<typename TNotification>
		class MockStatelessNotificationValidator : public BaseMockStatelessNotificationValidator<TNotification> {
		public:
			using BaseMockStatelessNotificationValidator<TNotification>::BaseMockStatelessNotificationValidator;
		};

		template<>
		class MockStatelessNotificationValidator<model::AggregateCosignaturesNotification<1>>
			: public BaseMockStatelessNotificationValidator<model::AggregateCosignaturesNotification<1>> {
		public:
			using BaseMockStatelessNotificationValidator<model::AggregateCosignaturesNotification<1>>::BaseMockStatelessNotificationValidator;

		public:
			ValidationResult validate(const model::AggregateCosignaturesNotification<1>& notification) const override {
				return HasAllCosignatures(notification) ? ValidationResult::Success : validators::Failure_Aggregate_Missing_Cosigners;
			}
		};

		template<>
		class MockStatelessNotificationValidator<model::AggregateEmbeddedTransactionNotification<1>>
			: public BaseMockStatelessNotificationValidator<model::AggregateEmbeddedTransactionNotification<1>> {
		public:
			using BaseMockStatelessNotificationValidator<model::AggregateEmbeddedTransactionNotification<1>>::BaseMockStatelessNotificationValidator;

		public:
			ValidationResult validate(const model::AggregateEmbeddedTransactionNotification<1>& notification) const override {
				return HasAllCosignatures(notification) ? ValidationResult::Success : validators::Failure_Aggregate_Ineligible_Cosigners;
			}
		};

		template<typename TNotification>
		std::unique_ptr<const validators::NotificationValidatorT<TNotification>> CreateStatelessValidator(ValidationResult validationResult) {
			return std::make_unique<MockStatelessNotificationValidator<TNotification>>(validationResult);
		}

		template<typename TNotification, typename TBuilder>
		void AddStatelessValidator(TBuilder& builder, ValidationResult validationResult) {
			builder.add(CreateStatelessValidator<TNotification>(validationResult));
		}

		class TestContext : public test::ServiceLocatorTestContext<PtDispatcherServiceTraits> {
		public:
			TestContext() : TestContext(ValidationResult::Failure)
			{}

			explicit TestContext(ValidationResult validationResult)
					: m_numCompletedTransactions(0)
					, m_pWriters(std::make_shared<mocks::BroadcastAwareMockPacketWriters>()) {
				auto pBootstrapperRegistrar = CreatePtBootstrapperServiceRegistrar([]() {
					return std::make_unique<cache::MemoryPtCacheProxy>(cache::MemoryCacheOptions(1024, 1024),
																	   std::make_shared<model::TransactionFeeCalculator>());
				});
				pBootstrapperRegistrar->registerServices(locator(), testState().state());

				// register mock packet writers
				locator().registerService(Service_Name, m_pWriters);

				// pt updater supports only aggregate transactions and tests check that txes are forwarded to pt updater
				// Custom_Buffers is needed to make sure cosignatures won't be included in hash calculation
				auto& pluginManager = testState().pluginManager();
				constexpr auto Not_Embeddable = utils::to_underlying_type(mocks::PluginOptionFlags::Not_Embeddable);
				constexpr auto Custom_Buffers = utils::to_underlying_type(mocks::PluginOptionFlags::Custom_Buffers);

				pluginManager.addTransactionSupport(mocks::CreateMockTransactionPlugin(
						Transaction_Type,
						static_cast<mocks::PluginOptionFlags>(Not_Embeddable | Custom_Buffers)));
				pluginManager.addTransactionSupport(mocks::CreateMockTransactionPlugin());

				pluginManager.addStatelessValidatorHook([validationResult](auto& builder) {
					AddStatelessValidator<model::AggregateCosignaturesNotification<1>>(builder, validationResult);
					AddStatelessValidator<model::AggregateEmbeddedTransactionNotification<1>>(builder, validationResult);
					AddStatelessValidator<model::EntityNotification<1>>(builder, validationResult);
					AddStatelessValidator<model::TransactionNotification<1>>(builder, validationResult);
					AddStatelessValidator<model::TransactionDeadlineNotification<1>>(builder, validationResult);
					AddStatelessValidator<model::TransactionFeeNotification<1>>(builder, validationResult);
					AddStatelessValidator<model::BalanceDebitNotification<1>>(builder, validationResult);
					AddStatelessValidator<model::SignatureNotification<1>>(builder, validationResult);
				});

				testState().state().hooks().setTransactionRangeConsumerFactory([&counter = m_numCompletedTransactions](auto) {
					return [&counter](auto&& range) { counter += range.Range.size(); };
				});
			}

		public:
			const cache::MemoryPtCacheProxy& cache() const {
				return GetMemoryPtCache(locator());
			}

			size_t numCompletedTransactions() const {
				return m_numCompletedTransactions;
			}

			const auto& registry() {
				return testState().pluginManager().transactionRegistry();
			}

			size_t numBroadcastCalls() const {
				return m_pWriters->numBroadcastCalls();
			}

			const std::vector<ionet::PacketPayload>& broadcastedPayloads() const {
				return m_pWriters->broadcastedPayloads();
			}

		private:
			std::atomic<size_t> m_numCompletedTransactions;
			std::shared_ptr<mocks::BroadcastAwareMockPacketWriters> m_pWriters;
		};

		Hash256 CalculateTransactionHash(const model::TransactionRegistry& registry, const model::Transaction& transaction) {
			const auto& plugin = *registry.findPlugin(transaction.Type);
			return model::CalculateHash(transaction, test::GetNemesisGenerationHash(), plugin.dataBuffer(transaction));
		}

		std::vector<Hash256> CalculateHashes(const model::TransactionRegistry& registry, const model::TransactionRange& transactionRange) {
			std::vector<Hash256> hashes;
			for (const auto& transaction : transactionRange)
				hashes.push_back(CalculateTransactionHash(registry, transaction));

			return hashes;
		}
	}

	ADD_SERVICE_REGISTRAR_INFO_TEST(PtDispatcher, Post_Range_Consumers)

	TEST(TEST_CLASS, CanBootService) {
		// Arrange:
		TestContext context;

		// Act:
		context.boot();

		// Assert:
		EXPECT_EQ(Num_Expected_Services, context.locator().numServices());
		EXPECT_EQ(Num_Expected_Counters, context.locator().counters().size());
		EXPECT_EQ(Num_Expected_Tasks, context.testState().state().tasks().size());

		// - all services should exist
		EXPECT_TRUE(!!context.locator().service<disruptor::ConsumerDispatcher>("pt.dispatcher"));
		EXPECT_TRUE(!!context.locator().service<disruptor::ConsumerDispatcher>("pt.dispatcher.batch"));

		// - all counters should exist
		EXPECT_EQ(0u, context.counter(Counter_Name));
		EXPECT_EQ(0u, context.counter(Active_Counter_Name));

		// - partial transaction dispatcher should be initialized
		auto pDispatcher = context.locator().service<disruptor::ConsumerDispatcher>("pt.dispatcher");
		EXPECT_EQ("partial transaction dispatcher", pDispatcher->name());
		EXPECT_EQ(3u, pDispatcher->size());
		EXPECT_TRUE(pDispatcher->isRunning());

		// - nothing was broadcasted
		EXPECT_EQ(0u, context.numBroadcastCalls());

		// - no ranges have been completed
		EXPECT_EQ(0u, context.numCompletedTransactions());
	}

	TEST(TEST_CLASS, CanShutdownService) {
		// Arrange:
		TestContext context;

		// Act:
		context.boot();
		context.shutdown();

		// Assert:
		EXPECT_EQ(Num_Expected_Services, context.locator().numServices());
		EXPECT_EQ(Num_Expected_Counters, context.locator().counters().size());
		EXPECT_EQ(Num_Expected_Tasks, context.testState().state().tasks().size());

		// - only rooted services exist
		EXPECT_FALSE(!!context.locator().service<disruptor::ConsumerDispatcher>("pt.dispatcher"));
		EXPECT_TRUE(!!context.locator().service<void>("pt.dispatcher.batch"));

		// - all counters should indicate shutdown
		EXPECT_EQ(Sentinel_Counter_Value, context.counter(Counter_Name));
		EXPECT_EQ(Sentinel_Counter_Value, context.counter(Active_Counter_Name));

		// - nothing was broadcasted
		EXPECT_EQ(0u, context.numBroadcastCalls());

		// - no ranges have been completed
		EXPECT_EQ(0u, context.numCompletedTransactions());
	}

	namespace {
		// Tests are using mock transaction plugin with Custom_Buffer option.
		// We need to set proper value in mockTransaction.Data.Size, so that cosignatures won't be included.
		auto FixAggregateTransactionDataSize(model::AggregateTransaction& aggregateTransaction) {
			static_assert(
					sizeof(model::AggregateTransaction) < sizeof(mocks::MockTransaction),
					"this test requires mockTransaction Data to fit inside the aggregate transaction Payload");
			if (sizeof(model::AggregateTransaction) + aggregateTransaction.PayloadSize < sizeof(mocks::MockTransaction))
				CATAPULT_THROW_RUNTIME_ERROR("this test requires mockTransaction Data to fit inside the aggregate transaction Payload");

			auto* pTransactionData = reinterpret_cast<uint8_t*>(&aggregateTransaction);
			auto& mockTransaction = reinterpret_cast<mocks::MockTransaction&>(*pTransactionData);
			auto headerSizeDifference = sizeof(mocks::MockTransaction) - sizeof(model::AggregateTransaction);
			mockTransaction.Data.Size = static_cast<uint16_t>(aggregateTransaction.PayloadSize - headerSizeDifference);
		}

		auto CreateRandomAggregateTransaction(const model::TransactionRegistry& registry, bool validCosignatures) {
			auto pTransaction = test::CreateRandomAggregateTransactionWithCosignatures(Num_Cosignatures);
			FixAggregateTransactionDataSize(*pTransaction);

			auto aggregateHash = CalculateTransactionHash(registry, *pTransaction);
			if (validCosignatures)
				test::FixCosignatures(aggregateHash, *pTransaction);

			return pTransaction;
		}

		model::TransactionRange CreateAggregateTransactionRange(
				const model::TransactionRegistry& registry,
				size_t numTransactions,
				bool validCosignatures) {
			std::vector<model::TransactionRange> range;
			for (auto i = 0u; i < numTransactions; ++i)
				range.push_back(model::TransactionRange::FromEntity(CreateRandomAggregateTransaction(registry, validCosignatures)));

			return model::TransactionRange::MergeRanges(std::move(range));
		}

		struct DispatcherTestOptions {
			validators::ValidationResult ValidationResult;
			size_t NumEntities;
			bool ValidCosignatures;
		};

		auto ProcessAndWait(disruptor::ConsumerDispatcher& dispatcher, model::TransactionRange&& range) {
			std::atomic_bool processed(false);
			disruptor::ConsumerCompletionResult result;
			auto input = disruptor::ConsumerInput(std::move(range), disruptor::InputSource::Unknown);
			dispatcher.processElement(std::move(input), [&processed, &result](auto, const auto& processingResult) {
				result = processingResult;
				processed = true;
			});

			WAIT_FOR_EXPR(!!processed);
			return result;
		}

		template<typename TWait, typename THandler>
		void AssertDispatcherForwarding(const DispatcherTestOptions& options, TWait wait, THandler handler) {
			// Arrange:
			TestContext context(options.ValidationResult);
			context.boot();

			const auto& transactionRegistry = context.registry();
			auto range = CreateAggregateTransactionRange(transactionRegistry, options.NumEntities, options.ValidCosignatures);
			auto expectedHashes = CalculateHashes(transactionRegistry, range);

			// Sanity:
			EXPECT_EQ(options.NumEntities, expectedHashes.size());

			// Act:
			auto pDispatcher = context.locator().service<disruptor::ConsumerDispatcher>("pt.dispatcher");
			ProcessAndWait(*pDispatcher, std::move(range));
			wait(context);

			// Assert: only single range has been pushed, with NumEntities elements
			EXPECT_EQ(1u, context.counter(Counter_Name));
			handler(context, expectedHashes);
		}
	}

	TEST(TEST_CLASS, Dispatcher_DoesNotForwardToUpdaterWhenValidationFailed) {
		// Arrange:
		AssertDispatcherForwarding(
				DispatcherTestOptions{ ValidationResult::Failure, 1, false },
				[](const auto&) {},
				[](const auto& context, const auto& expectedHashes) {
					// Assert:
					auto view = context.cache().view();
					EXPECT_EQ(0u, view.size());
					EXPECT_FALSE(!!view.find(expectedHashes[0]));

					// - note that broadcast is done before validation
					EXPECT_EQ(1u, context.numBroadcastCalls());
					EXPECT_EQ(0u, context.numCompletedTransactions());
				});
	}

	TEST(TEST_CLASS, Dispatcher_ForwardsToUpdaterWhenValidationSucceeded) {
		AssertDispatcherForwarding(
				DispatcherTestOptions{ ValidationResult::Success, 1, false },
				[](const auto& context) {
					// wait for element processing to finish
					WAIT_FOR_ONE_EXPR(context.cache().view().size());
				},
				[](const auto& context, const auto& expectedHashes) {
					// Assert:
					auto view = context.cache().view();
					EXPECT_EQ(1u, view.size());
					EXPECT_TRUE(!!view.find(expectedHashes[0]));

					EXPECT_EQ(1u, context.numBroadcastCalls());
					EXPECT_EQ(0u, context.numCompletedTransactions());
				});
	}

	TEST(TEST_CLASS, Dispatcher_ForwardsMultipleEntitiesToUpdaterWhenValidationSucceeded) {
		AssertDispatcherForwarding(
				DispatcherTestOptions{ ValidationResult::Success, 3, false },
				[](const auto& context) {
					// wait for element processing to finish
					WAIT_FOR_VALUE_EXPR(3u, context.cache().view().size());
				},
				[](const auto& context, const auto& expectedHashes) {
					// Assert:
					auto view = context.cache().view();
					EXPECT_EQ(3u, view.size());
					for (const auto& expectedHash : expectedHashes)
						EXPECT_TRUE(!!view.find(expectedHash));

					EXPECT_EQ(1u, context.numBroadcastCalls());
					EXPECT_EQ(0u, context.numCompletedTransactions());
				});
	}

	TEST(TEST_CLASS, Dispatcher_PtCacheIgnoresTransactionWithSameHash) {
		// Arrange: disable short lived cache, so that dispatcher won't eliminate second element via recency cache
		TestContext context(ValidationResult::Success);
		const_cast<config::NodeConfiguration&>(context.testState().config().Node).ShortLivedCacheMaxSize = 0;
		context.boot();

		const auto& transactionRegistry = context.registry();
		auto range1 = CreateAggregateTransactionRange(transactionRegistry, 1, false);
		auto range2 = model::TransactionRange::CopyRange(range1);
		const auto& transaction = *range1.begin();
		auto expectedHash = CalculateTransactionHash(transactionRegistry, transaction);

		// - push first range into dispatcher
		auto pDispatcher = context.locator().service<disruptor::ConsumerDispatcher>("pt.dispatcher");
		ProcessAndWait(*pDispatcher, std::move(range1));

		// Sanity:
		{
			auto view = context.cache().view();
			EXPECT_EQ(1u, view.size());
			const auto& weakEntityInfo = view.find(expectedHash);
			EXPECT_TRUE(weakEntityInfo);

			EXPECT_EQ(1u, context.numBroadcastCalls());
		}

		// Act:
		auto result = ProcessAndWait(*pDispatcher, std::move(range2));

		// Assert: transaction was not broadcasted again
		EXPECT_EQ(2u, context.counter(Counter_Name));
		EXPECT_EQ(1u, context.numBroadcastCalls());
		EXPECT_EQ(0u, context.numCompletedTransactions());

		EXPECT_EQ(disruptor::CompletionStatus::Aborted, result.CompletionStatus);
		EXPECT_EQ(utils::to_underlying_type(consumers::Neutral_Consumer_Hash_In_Recency_Cache), result.CompletionCode);
	}

	TEST(TEST_CLASS, Dispatcher_UpdaterForwardsToConsumerWhenValidationSucceeded) {
		AssertDispatcherForwarding(
				DispatcherTestOptions{ ValidationResult::Success, 3, true },
				[](const auto& context) {
					// wait for element processing to finish
					WAIT_FOR_VALUE_EXPR(3u, context.numCompletedTransactions());
					WAIT_FOR_ONE_EXPR(context.numBroadcastCalls());
				},
				[](const auto& context, const auto& expectedHashes) {
					// Assert: should not be in cache any more
					auto view = context.cache().view();
					EXPECT_EQ(0u, view.size());
					EXPECT_FALSE(!!view.find(expectedHashes[0]));

					EXPECT_EQ(1u, context.numBroadcastCalls());
					EXPECT_EQ(3u, context.numCompletedTransactions());
				});
	}

	// region hooks

	namespace {
		auto ExtractTransactionPayload(const model::TransactionRange& range) {
			std::vector<model::TransactionInfo> transactionInfos;
			for (const auto& transaction : range)
				transactionInfos.push_back(model::TransactionInfo(std::shared_ptr<const model::Transaction>(&transaction, [](auto) {}), Height()));

			return ionet::CreateBroadcastPayload(transactionInfos, ionet::PacketType::Push_Partial_Transactions);
		}

		template<typename TInvokeHook>
		void AssertInvokeForwardsTransactionRangeToDispatcher(TInvokeHook invokeHook) {
			// Arrange:
			TestContext context(ValidationResult::Success);
			context.boot();

			// - prepare a transaction range
			const auto& transactionRegistry = context.registry();
			auto transactionRange = CreateAggregateTransactionRange(transactionRegistry, 3, false);
			auto expectedHashes = CalculateHashes(transactionRegistry, transactionRange);
			auto expectedPayload = ExtractTransactionPayload(transactionRange);

			// Act:
			invokeHook(GetPtServerHooks(context.locator()), std::move(transactionRange));
			context.testState().state().tasks()[0].Callback(); // forward all batched transactions to the dispatcher

			// Assert: wait for element processing to finish
			WAIT_FOR_VALUE_EXPR(3u, context.cache().view().size());
			WAIT_FOR_ONE_EXPR(context.numBroadcastCalls());

			// - cache should be populated with range entities
			auto view = context.cache().view();
			EXPECT_EQ(3u, view.size());

			auto i = 0u;
			for (const auto& hash : expectedHashes) {
				EXPECT_TRUE(!!view.find(hash)) << "hash at " << std::to_string(i);
				++i;
			}

			ASSERT_EQ(1u, context.numBroadcastCalls());
			test::AssertEqualPayload(expectedPayload, context.broadcastedPayloads()[0]);

			EXPECT_EQ(0u, context.numCompletedTransactions());
		}
	}

	TEST(TEST_CLASS, CosignedTransactionInfosConsumerForwardsTransactionRangeToDispatcher) {
		// Assert:
		AssertInvokeForwardsTransactionRangeToDispatcher([](const auto& hooks, auto&& transactionRange) {
			// Arrange:
			CosignedTransactionInfos transactionInfos;
			for (const auto& transaction : transactionRange) {
				auto transactionInfo = model::CosignedTransactionInfo();
				transactionInfo.pTransaction = std::shared_ptr<const model::Transaction>(&transaction, [](const auto*) {});
				transactionInfos.push_back(transactionInfo);
			}

			// Act:
			hooks.cosignedTransactionInfosConsumer()(std::move(transactionInfos));
		});
	}

	TEST(TEST_CLASS, PtRangeConsumerForwardsTransactionRangeToDispatcher) {
		// Assert:
		AssertInvokeForwardsTransactionRangeToDispatcher([](const auto& hooks, auto&& transactionRange) {
			// Act:
			hooks.ptRangeConsumer()(std::move(transactionRange));
		});
	}

	namespace {
		auto ExtractCosignaturePayload(const model::EntityRange<model::DetachedCosignature>& range) {
			std::vector<model::DetachedCosignature> cosignatures;
			for (const auto& cosignature : range)
				cosignatures.push_back(cosignature);

			return ionet::CreateBroadcastPayload(cosignatures);
		}

		template<typename TInvokeHook>
		void AssertInvokeForwardsCosignatureRangeToUpdater(TInvokeHook invokeHook) {
			// Arrange:
			TestContext context(ValidationResult::Success);
			context.boot();

			auto& ptCache = GetMemoryPtCache(context.locator());

			// - prepare and add a transaction range to the cache
			auto pTransaction = utils::UniqueToShared(test::CreateAggregateTransaction(1).pTransaction);
			FixAggregateTransactionDataSize(*pTransaction);

			model::TransactionInfo transactionInfo;
			transactionInfo.EntityHash = CalculateTransactionHash(context.registry(), *pTransaction);
			transactionInfo.pEntity = pTransaction;
			ptCache.modifier().add(transactionInfo);

			// - prepare a cosignature range
			auto cosignatureRange = model::EntityRange<model::DetachedCosignature>::PrepareFixed(3);
			std::vector<Key> cosigners;
			for (auto& cosignature : cosignatureRange) {
				cosignature = test::GenerateValidCosignature(transactionInfo.EntityHash);
				cosigners.push_back(cosignature.Signer);
			}

			auto expectedPayload = ExtractCosignaturePayload(cosignatureRange);

			// Act:
			invokeHook(GetPtServerHooks(context.locator()), std::move(cosignatureRange));

			// Assert: wait for element processing to finish
			WAIT_FOR_VALUE_EXPR(3u, context.cache().view().find(transactionInfo.EntityHash).cosignatures().size());
			WAIT_FOR_ONE_EXPR(context.numBroadcastCalls());

			// - cache should be populated with cosignatures
			auto view = context.cache().view();
			EXPECT_EQ(1u, view.size());

			auto i = 0u;
			auto transactionInfoFromCache = view.find(transactionInfo.EntityHash);
			ASSERT_TRUE(!!transactionInfoFromCache);
			for (const auto& cosigner : cosigners) {
				EXPECT_TRUE(transactionInfoFromCache.hasCosigner(cosigner)) << "cosigner at " << std::to_string(i);
				++i;
			}

			ASSERT_EQ(1u, context.numBroadcastCalls());
			test::AssertEqualPayload(expectedPayload, context.broadcastedPayloads()[0]);

			EXPECT_EQ(0u, context.numCompletedTransactions());
		}
	}

	TEST(TEST_CLASS, CosignedTransactionInfosConsumerForwardsCosignaturesToUpdater) {
		// Assert:
		AssertInvokeForwardsCosignatureRangeToUpdater([](const auto& hooks, auto&& cosignatureRange) {
			// Arrange: create a separate info for each cosignature
			CosignedTransactionInfos transactionInfos;
			for (const auto& cosignature : cosignatureRange) {
				auto transactionInfo = model::CosignedTransactionInfo();
				transactionInfo.EntityHash = cosignature.ParentHash;
				transactionInfo.Cosignatures.push_back(cosignature);
				transactionInfos.push_back(transactionInfo);
			}

			// Act:
			hooks.cosignedTransactionInfosConsumer()(std::move(transactionInfos));
		});
	}

	TEST(TEST_CLASS, CosignatureRangeConsumerForwardsCosignatureRangeToUpdater) {
		// Assert:
		AssertInvokeForwardsCosignatureRangeToUpdater([](const auto& hooks, auto&& cosignatureRange) {
			// Act:
			hooks.cosignatureRangeConsumer()(std::move(cosignatureRange));
		});
	}

	// endregion
}}
