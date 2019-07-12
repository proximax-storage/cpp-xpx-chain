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

#include "catapult/disruptor/ConsumerDispatcher.h"
#include "catapult/model/RangeTypes.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/nodeps/Atomics.h"
#include "tests/test/nodeps/Functional.h"
#include "tests/test/other/DisruptorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace disruptor {

#define TEST_CLASS ConsumerDispatcherTests

	namespace {
		static constexpr ConsumerDispatcherOptions Test_Dispatcher_Options{ "ConsumerDispatcherTests", 16u * 1024 };

		auto CreateNoOpConsumer() {
			return [](const auto&) {
				return ConsumerResult::Continue();
			};
		}

		void ProcessAll(ConsumerDispatcher& dispatcher, std::vector<model::BlockRange>&& ranges) {
			for (auto& range : ranges)
				dispatcher.processElement(ConsumerInput(std::move(range)));
		}

		void AssertHasProcessedNoElements(ConsumerDispatcher& dispatcher) {
			EXPECT_EQ(0u, dispatcher.numAddedElements());
			EXPECT_EQ(0u, dispatcher.numActiveElements());
			EXPECT_TRUE(dispatcher.isRunning());
		}
	}

	// region create + shutdown

	TEST(TEST_CLASS, CannotCreateDispatcherWithNullName) {
		// Arrange:
		auto options = Test_Dispatcher_Options;
		options.DispatcherName = nullptr;

		// Act + Assert:
		EXPECT_THROW(ConsumerDispatcher(options, {}), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, CannotCreateDispatcherWithZeroSizeDisruptor) {
		// Arrange:
		auto options = Test_Dispatcher_Options;
		options.DisruptorSize = 0;

		// Act + Assert:
		EXPECT_THROW(ConsumerDispatcher(options, {}), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, CanCreateEmptyDispatcher) {
		// Arrange:
		ConsumerDispatcher dispatcher(Test_Dispatcher_Options, {});

		// Assert:
		EXPECT_EQ(Test_Dispatcher_Options.DispatcherName, dispatcher.name());
		EXPECT_EQ(0u, dispatcher.size());
		AssertHasProcessedNoElements(dispatcher);
	}

	TEST(TEST_CLASS, CanCreateDispatcherWithConsumer) {
		// Arrange + Act:
		ConsumerDispatcher dispatcher(Test_Dispatcher_Options, { CreateNoOpConsumer() });

		// Assert:
		EXPECT_EQ(Test_Dispatcher_Options.DispatcherName, dispatcher.name());
		EXPECT_EQ(1u, dispatcher.size());
		AssertHasProcessedNoElements(dispatcher);
	}

	TEST(TEST_CLASS, ShutdownStopsDispatcher) {
		// Arrange:
		auto numConsumerCalls = 0u;
		auto numInspectorCalls = 0u;
		auto ranges = test::PrepareRanges(1);
		ConsumerDispatcher dispatcher(
				Test_Dispatcher_Options,
				{
					[&numConsumerCalls](const auto&) {
						++numConsumerCalls;
						return ConsumerResult::Continue();
					}
				},
				[&numInspectorCalls](const auto&, const auto&) { ++numInspectorCalls; });

		// Act:
		dispatcher.shutdown();
		ProcessAll(dispatcher, std::move(ranges));

		// Assert:
		EXPECT_EQ(1u, dispatcher.size());
		EXPECT_EQ(ranges.size(), dispatcher.numAddedElements());
		EXPECT_EQ(0u, numConsumerCalls);
		EXPECT_FALSE(dispatcher.isRunning());
		EXPECT_EQ(0u, numInspectorCalls);
	}

	// endregion

	// region completion handler

	namespace {
		auto CreateAlternatingResultConsumer() {
			auto pCounter = std::make_shared<uint8_t>(static_cast<uint8_t>(0));
			return [pCounter](const auto&) {
				auto id = ++*pCounter;
				return 0 == id % 2 ? ConsumerResult::Continue() : ConsumerResult::Abort(id * id);
			};
		}
	}

	TEST(TEST_CLASS, CompletionHandlerIsCalledWhenProcessingIsComplete) {
		// Arrange:
		ConsumerDispatcher dispatcher(Test_Dispatcher_Options, { CreateAlternatingResultConsumer() });
		auto ranges = test::PrepareRanges(3);
		std::vector<DisruptorElementId> elementIds;
		std::vector<DisruptorElementId> capturedElementIds;
		std::vector<ConsumerCompletionResult> capturedResults;

		// Act:
		std::atomic<size_t> numCapturedElements(0);
		for (auto i = 0u; i < ranges.size(); ++i) {
			// note: the alternating result consumer starts with ConsumerResult::Abort()
			auto processingCompleteCallback = [&capturedElementIds, &capturedResults, &numCapturedElements](auto id, const auto& result) {
				capturedElementIds.push_back(id);
				capturedResults.push_back(result);
				++numCapturedElements;
			};
			elementIds.push_back(dispatcher.processElement(ConsumerInput(std::move(ranges[i])), processingCompleteCallback));
		}

		WAIT_FOR_VALUE(ranges.size(), numCapturedElements);

		// Assert:
		std::vector<DisruptorElementId> expectedElementIds{ 1u, 2u, 3u };
		EXPECT_EQ(Test_Dispatcher_Options.DispatcherName, dispatcher.name());
		EXPECT_EQ(1u, dispatcher.size());
		EXPECT_EQ(3u, dispatcher.numAddedElements());
		EXPECT_TRUE(dispatcher.isRunning());
		EXPECT_EQ(expectedElementIds, elementIds);
		EXPECT_EQ(expectedElementIds, capturedElementIds);
		EXPECT_EQ(ranges.size(), capturedElementIds.size());

		ASSERT_EQ(ranges.size(), capturedResults.size());
		test::AssertAborted(capturedResults[0], 1, 0);
		test::AssertContinued(capturedResults[1]);
		test::AssertAborted(capturedResults[2], 9, 2);
	}

	TEST(TEST_CLASS, CompletionHandlerIsNotCalledWhenInputIsEmpty) {
		// Arrange:
		ConsumerDispatcher dispatcher(Test_Dispatcher_Options, { CreateNoOpConsumer() });
		auto handlerCallCount = 0u;

		// Act:
		auto elementId = dispatcher.processElement(ConsumerInput(model::BlockRange()), [&handlerCallCount](auto, const auto&) {
			++handlerCallCount;
		});

		// Assert:
		EXPECT_EQ(Test_Dispatcher_Options.DispatcherName, dispatcher.name());
		EXPECT_EQ(1u, dispatcher.size());
		AssertHasProcessedNoElements(dispatcher);
		EXPECT_EQ(0u, elementId);
		EXPECT_EQ(0u, handlerCallCount);
	}

	// endregion

	// region numActiveElements

	TEST(TEST_CLASS, NumActiveElementsReportsNumberOfInProgressElements) {
		// Arrange:
		test::AutoSetFlag isExecutingBlockedElementCallback;
		test::AutoSetFlag isElementCallbackUnblocked;
		auto pIsExecuting = isExecutingBlockedElementCallback.state();
		auto pIsUnblocked = isElementCallbackUnblocked.state();

		auto ranges = test::PrepareRanges(6);
		ConsumerDispatcher dispatcher(Test_Dispatcher_Options, { CreateNoOpConsumer() });

		// Act: block the third element
		for (auto i = 0u; i < 6; ++i) {
			auto input = ConsumerInput(std::move(ranges[i]));
			if (2 != i) {
				dispatcher.processElement(std::move(input));
				continue;
			}

			dispatcher.processElement(std::move(input), [pIsExecuting, pIsUnblocked](auto, const auto&) {
				pIsExecuting->set();
				pIsUnblocked->wait();
			});
		}

		// - wait until the blocked element callback is called
		isExecutingBlockedElementCallback.state()->wait();

		// Assert: all elements should be added and four should be active
		EXPECT_EQ(6u, dispatcher.numAddedElements());
		EXPECT_EQ(4u, dispatcher.numActiveElements());

		// Act: allow all elements to complete
		isElementCallbackUnblocked.state()->set();
		WAIT_FOR_ZERO_EXPR(dispatcher.numActiveElements());

		// Assert: no elements should be active
		EXPECT_EQ(6u, dispatcher.numAddedElements());
		EXPECT_EQ(0u, dispatcher.numActiveElements());
	}

	// endregion

	// region process + consume (no inspect)

	namespace {
		auto GetExpectedHeights(const std::vector<model::BlockRange>& ranges) {
			return test::Apply(true, ranges, [](const auto& range) {
				return test::Apply(true, range, [](const auto& block) {
					return block.Height;
				});
			});
		}

		auto BlockElementVectorToHeights(BlockElements& entities) {
			return test::Apply(true, entities, [](const auto& blockElement) {
				return blockElement.Block.Height;
			});
		}

		using Heights = std::vector<Height>;

		auto CreateConsumer(std::vector<Heights>& collector) {
			return [&](auto& consumerInput) {
				auto heights = BlockElementVectorToHeights(consumerInput.blocks());
				collector.push_back(heights);
				return ConsumerResult::Continue();
			};
		}
	}

	TEST(TEST_CLASS, ProcessElementReturnsElementId) {
		// Arrange:
		auto ranges = test::PrepareRanges(6);
		ConsumerDispatcher dispatcher(Test_Dispatcher_Options, { CreateNoOpConsumer() });
		std::vector<DisruptorElementId> ids;
		std::vector<DisruptorElementId> expectedIds{ 1, 2, 3, 4, 5, 6 };

		// Act: push single element
		for (auto i = 0u; i < 6; ++i) {
			if (0 == i % 2)
				ids.push_back(dispatcher.processElement(ConsumerInput(std::move(ranges[i]))));
			else
				ids.push_back(dispatcher.processElement(ConsumerInput(std::move(ranges[i])), [](auto, const auto&) {}));
		}

		// Assert:
		EXPECT_EQ(ranges.size(), dispatcher.numAddedElements());
		EXPECT_EQ(expectedIds, ids);
	}

	TEST(TEST_CLASS, ConsumerIsNotPassedEmptyInput) {
		// Arrange:
		std::vector<model::BlockRange> ranges;
		ranges.push_back(model::BlockRange());

		auto numConsumerCalls = 0u;
		auto numInspectorCalls = 0u;
		ConsumerDispatcher dispatcher(
				Test_Dispatcher_Options,
				{
					[&numConsumerCalls](const auto&) {
						++numConsumerCalls;
						return ConsumerResult::Continue();
					}
				},
				[&numInspectorCalls](const auto&, const auto&) { ++numInspectorCalls; });

		// Act: push single (empty) element
		ProcessAll(dispatcher, std::move(ranges));
		// - pause is here, to let both the consumer and inspector continue for a bit more,
		//   in case if it would still be running and there'd be bug in implementation
		test::Pause();

		// Assert: the element was not passed to the consumer or inspector
		EXPECT_EQ(0u, dispatcher.numAddedElements());
		EXPECT_EQ(0u, dispatcher.numActiveElements());
		EXPECT_EQ(0u, numConsumerCalls);
		EXPECT_EQ(0u, numInspectorCalls);
	}

	TEST(TEST_CLASS, ConsumerCanConsumeSingleElement) {
		// Arrange:
		auto ranges = test::PrepareRanges(1);
		auto expectedHeights = GetExpectedHeights(ranges);

		std::vector<Heights> collectedHeights;
		ConsumerDispatcher dispatcher(Test_Dispatcher_Options, { CreateConsumer(collectedHeights) });

		// Act: push single element
		ProcessAll(dispatcher, std::move(ranges));
		WAIT_FOR_ONE_EXPR(collectedHeights.size());

		// Assert:
		EXPECT_EQ(1u, collectedHeights.size());
		EXPECT_EQ(ranges.size(), dispatcher.numAddedElements());
		EXPECT_EQ(expectedHeights, collectedHeights);
	}

	// endregion

	// region inspect + consume

	TEST(TEST_CLASS, CanInspectSingleElement) {
		// Arrange:
		auto ranges = test::PrepareRanges(1);
		auto expectedHeights = GetExpectedHeights(ranges);

		std::vector<Heights> collectedHeights;
		auto numInspectorCalls = 0u;
		ConsumerDispatcher dispatcher(
				Test_Dispatcher_Options,
				{ CreateConsumer(collectedHeights) },
				[&numInspectorCalls](const auto&, const auto&) { ++numInspectorCalls; });

		// Act: push single element
		ProcessAll(dispatcher, std::move(ranges));
		WAIT_FOR_ONE_EXPR(numInspectorCalls);
		// - pause is here, to let both the consumer and inspector continue for a bit more,
		//   in case if it would still be running and there'd be bug in implementation
		test::Pause();

		// Assert:
		EXPECT_EQ(1u, collectedHeights.size());
		EXPECT_EQ(ranges.size(), dispatcher.numAddedElements());
		EXPECT_EQ(0u, dispatcher.numActiveElements());
		EXPECT_EQ(expectedHeights, collectedHeights);
		EXPECT_EQ(1u, numInspectorCalls);
	}

	namespace {
		auto CreateCollectingInspector(std::vector<Heights>& heightsCollector, std::vector<CompletionStatus>& statusCollector) {
			return [&](auto& input, const auto& completionResult) {
				heightsCollector.push_back(BlockElementVectorToHeights(input.blocks()));
				statusCollector.push_back(completionResult.CompletionStatus);
			};
		}
	}

	TEST(TEST_CLASS, CanConsumeAndInspectAllElementsWithSingleConsumer) {
		// Arrange:
		auto ranges = test::PrepareRanges(5);
		auto expectedHeights = GetExpectedHeights(ranges);
		std::vector<Heights> collectedHeights;
		std::vector<Heights> inspectedHeights;
		std::vector<CompletionStatus> inspectedStatuses;

		// Act:
		ConsumerDispatcher dispatcher(
				Test_Dispatcher_Options,
				{ CreateConsumer(collectedHeights) },
				CreateCollectingInspector(inspectedHeights, inspectedStatuses));

		// - push multiple elements
		ProcessAll(dispatcher, std::move(ranges));
		WAIT_FOR_VALUE_EXPR(5u, inspectedHeights.size());
		WAIT_FOR_ZERO_EXPR(dispatcher.numActiveElements());

		// Assert:
		EXPECT_EQ(ranges.size(), dispatcher.numAddedElements());
		EXPECT_EQ(0u, dispatcher.numActiveElements());
		EXPECT_EQ(expectedHeights, collectedHeights);
		EXPECT_EQ(expectedHeights, inspectedHeights);
		EXPECT_EQ(std::vector<CompletionStatus>(5, CompletionStatus::Normal), inspectedStatuses);
	}

	TEST(TEST_CLASS, CanConsumeAndInspectAllElementsWithMultipleConsumers) {
		// Arrange:
		auto ranges = test::PrepareRanges(5);
		auto expectedHeights = GetExpectedHeights(ranges);
		std::vector<Heights> collectedHeights[3];
		std::vector<Heights> inspectedHeights;
		std::vector<CompletionStatus> inspectedStatuses;

		// Act:
		ConsumerDispatcher dispatcher(
				Test_Dispatcher_Options,
				{
					CreateConsumer(collectedHeights[0]),
					CreateConsumer(collectedHeights[1]),
					CreateConsumer(collectedHeights[2]),
				},
				CreateCollectingInspector(inspectedHeights, inspectedStatuses));

		// - push multiple elements
		ProcessAll(dispatcher, std::move(ranges));
		WAIT_FOR_VALUE_EXPR(5u, inspectedHeights.size());
		WAIT_FOR_ZERO_EXPR(dispatcher.numActiveElements());

		// Assert:
		EXPECT_EQ(ranges.size(), dispatcher.numAddedElements());
		EXPECT_EQ(0u, dispatcher.numActiveElements());
		EXPECT_EQ(expectedHeights, collectedHeights[0]);
		EXPECT_EQ(expectedHeights, collectedHeights[1]);
		EXPECT_EQ(expectedHeights, collectedHeights[2]);
		EXPECT_EQ(expectedHeights, inspectedHeights);
		EXPECT_EQ(std::vector<CompletionStatus>(5, CompletionStatus::Normal), inspectedStatuses);
	}

	// endregion

	// region element marking

	namespace {
		auto CreateSkipIfFirstBlockIsEvenConsumer() {
			return [](auto& consumerInput) {
				return 0 == consumerInput.blocks()[0].Block.Height.unwrap() % 2
						? ConsumerResult::Abort()
						: ConsumerResult::Continue();
			};
		}
	}

	TEST(TEST_CLASS, MarkedElementsAreSkippedByHigherConsumers) {
		// Arrange:
		std::vector<Heights> collectedHeights;
		auto ranges = test::PrepareRanges(5);
		auto height = 0u;
		for (auto& range : ranges)
			range.begin()->Height = Height(++height);

		auto expectedHeights = test::Filter(GetExpectedHeights(ranges), [](const auto& heights) {
			return 1 == heights[0].unwrap() % 2;
		});

		// Act:
		ConsumerDispatcher dispatcher(
				Test_Dispatcher_Options,
				{
					CreateSkipIfFirstBlockIsEvenConsumer(),
					CreateConsumer(collectedHeights),
				});

		// push multiple elements
		ProcessAll(dispatcher, std::move(ranges));
		WAIT_FOR_VALUE_EXPR(3u, collectedHeights.size());

		// Assert:
		EXPECT_EQ(ranges.size(), dispatcher.numAddedElements());
		EXPECT_EQ(expectedHeights, collectedHeights);
	}

	TEST(TEST_CLASS, MarkedElementsArePassedToInspector) {
		// Arrange:
		std::vector<Heights> inspectedHeights;
		std::vector<CompletionStatus> inspectedStatuses;
		auto ranges = test::PrepareRanges(5);
		auto height = 0u;
		for (auto& range : ranges)
			range.begin()->Height = Height(++height);

		auto expectedHeights = GetExpectedHeights(ranges);

		// Act:
		ConsumerDispatcher dispatcher(
				Test_Dispatcher_Options,
				{ CreateSkipIfFirstBlockIsEvenConsumer() },
				CreateCollectingInspector(inspectedHeights, inspectedStatuses));

		// - push multiple elements
		ProcessAll(dispatcher, std::move(ranges));
		WAIT_FOR_VALUE_EXPR(5u, inspectedHeights.size());

		// Assert:
		EXPECT_EQ(ranges.size(), dispatcher.numAddedElements());
		EXPECT_EQ(expectedHeights, inspectedHeights);

		// - ranges have heights 1-5, where even heights should be aborted
		auto expectedStatuses = std::vector<CompletionStatus>(5, CompletionStatus::Normal);
		expectedStatuses[1] = CompletionStatus::Aborted;
		expectedStatuses[3] = CompletionStatus::Aborted;
		EXPECT_EQ(expectedStatuses, inspectedStatuses);
	}

	// endregion

	// region exception + space exhaution

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4702) /* "unreachable code" */
#endif

	TEST(TEST_CLASS, ExceptionThrownFromTheConsumerTerminates) {
		ASSERT_DEATH({
			// Arrange:
			auto ranges = test::PrepareRanges(1);

			ConsumerDispatcher dispatcher(
					Test_Dispatcher_Options,
					{
						[](const auto&) {
							CATAPULT_THROW_RUNTIME_ERROR("dummy consumer exception");
							return ConsumerResult::Continue();
						}
					});

			// Act:
			ProcessAll(dispatcher, std::move(ranges));
			WAIT_FOR_EXPR(!dispatcher.isRunning());
		}, "");
	}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

	namespace {
		template<typename TAction>
		void RunDispatcherFullTest(const ConsumerDispatcherOptions& options, TAction action) {
			// Arrange:
			auto ranges = test::PrepareRanges(options.DisruptorSize - 1);
			std::atomic<size_t> counter(0);
			std::unique_ptr<ConsumerDispatcher> pDispatcher;
			test::AutoSetFlag continueFlag;
			pDispatcher = std::make_unique<ConsumerDispatcher>(options, std::vector<DisruptorConsumer>{
				[&counter](const auto&) {
					++counter;
					CATAPULT_LOG(info) << "consumer is processing element " << counter;
					return ConsumerResult::Continue();
				},
				[pContinueFlag = continueFlag.state()](const auto&) {
					pContinueFlag->wait();
					return ConsumerResult::Continue();
				}
			});

			// Act: first consumer is processing disruptorSize - 1 elements, second consumer just waits
			ProcessAll(*pDispatcher, std::move(ranges));
			WAIT_FOR_VALUE(options.DisruptorSize - 1, counter);

			// Assert:
			action(*pDispatcher, continueFlag);
		}
	}

	TEST(TEST_CLASS, DispatcherThrowsWhenDisruptorSpaceIsExhausted) {
		// Act:
		static constexpr auto Disruptor_Size = 8u;
		RunDispatcherFullTest({ "ConsumerDispatcherTests", Disruptor_Size }, [](auto& dispatcher, const auto&) {
			// Act + Assert: adding another element to the disruptor will fail
			CATAPULT_LOG(info) << "consumer attempting to process another element";
			EXPECT_THROW(dispatcher.processElement(ConsumerInput(test::CreateBlockEntityRange(1))), catapult_runtime_error);
		});
	}

	TEST(TEST_CLASS, DispatcherReturnsZeroWhenDisruptorSpaceIsExhaustedAndShouldThrowWhenFullIsFalse) {
		// Arrange:
		static constexpr auto Disruptor_Size = 8u;
		auto options = ConsumerDispatcherOptions{ "ConsumerDispatcherTests", Disruptor_Size };
		options.ShouldThrowWhenFull = false;

		// Act:
		RunDispatcherFullTest(options, [](auto& dispatcher, const auto&) {
			CATAPULT_LOG(info) << "consumer attempting to process another element";
			auto killerId = dispatcher.processElement(ConsumerInput(test::CreateBlockEntityRange(1)));

			// Assert: no element was added to disruptor
			EXPECT_EQ(0u, killerId);
			EXPECT_EQ(Disruptor_Size - 1, dispatcher.numAddedElements());
			EXPECT_EQ(Disruptor_Size - 1, dispatcher.numActiveElements());
		});
	}

	TEST(TEST_CLASS, DispatcherCanProcessAdditionalElementsAfterZeroIsReturnedDueToFullDisruptor) {
		// Arrange:
		static constexpr auto Disruptor_Size = 8u;
		auto options = ConsumerDispatcherOptions{ "ConsumerDispatcherTests", Disruptor_Size };
		options.ShouldThrowWhenFull = false;

		// Act:
		RunDispatcherFullTest(options, [](auto& dispatcher, auto& continueFlag) {
			CATAPULT_LOG(info) << "consumer attempting to process another element";
			auto killerId = dispatcher.processElement(ConsumerInput(test::CreateBlockEntityRange(1)));

			// Sanity: dispatcher is full
			EXPECT_EQ(0u, killerId);
			EXPECT_EQ(Disruptor_Size - 1, dispatcher.numAddedElements());

			// Act: drain the dispatcher
			continueFlag.state()->set();
			WAIT_FOR_ZERO_EXPR(dispatcher.numActiveElements());

			// - add another range
			auto lastId = dispatcher.processElement(ConsumerInput(test::CreateBlockEntityRange(1)));

			// Assert: the range was added
			EXPECT_EQ(Disruptor_Size, lastId);
			EXPECT_EQ(Disruptor_Size, dispatcher.numAddedElements());
		});
	}

	namespace {
		auto CreateConsumers(
				std::vector<size_t>& counters,
				std::atomic<size_t>& totalCounter,
				size_t offset,
				const std::shared_ptr<test::AutoSetFlag::State>& pContinueFlag) {
			std::vector<DisruptorConsumer> consumers(counters.size());
			for (auto i = 0u; i < counters.size(); ++i) {
				auto consumer = [&counters, &totalCounter, offset, pContinueFlag, i](const auto&) {
					CATAPULT_LOG(info) << "consumer " << i << " is processing element " << (counters[i] + 1);
					++counters[i];
					++totalCounter;
					if (counters[i] < counters.size() - i + offset - 1)
						return ConsumerResult::Continue();

					CATAPULT_LOG(info) << "consumer " << i << " reached blocking position";
					while (!pContinueFlag->isSet())
						test::Sleep(5);

					return ConsumerResult::Continue();
				};
				consumers[i] = std::move(consumer);
			}

			return consumers;
		}

		void AssertDeathIfDisruptorSpaceIsExhaustedEvenIfOtherConsumersHaveNearbyPositions(size_t offset) {
			// Arrange:
			constexpr auto Disruptor_Size = 4u;
			std::vector<size_t> counters(Disruptor_Size);
			std::atomic<size_t> totalCounter(0);
			std::unique_ptr<ConsumerDispatcher> pDispatcher;
			test::AutoSetFlag continueFlag;
			auto consumers = CreateConsumers(counters, totalCounter, offset, continueFlag.state());
			pDispatcher = std::make_unique<ConsumerDispatcher>(
					ConsumerDispatcherOptions{ "ConsumerDispatcherTests", Disruptor_Size },
					consumers);

			// - let consumers process offset ranges, their position is at offset afterwards
			//   (except for the inspector which is at position offset - 1)
			auto phase1Ranges = test::PrepareRanges(offset);
			for (auto i = 0u; i < offset; ++i) {
				pDispatcher->processElement(ConsumerInput(std::move(phase1Ranges[i])));

				// wait for each element to be fully processed so consumer 0 does not get too far ahead
				WAIT_FOR_VALUE((i + 1) * Disruptor_Size, totalCounter);
			}

			// Act:
			// - consumer 0 + 1 will advance Disruptor_Size - 2 positions
			// - consumer 2 will advance Disruptor_Size - 3 positions
			// - consumer 3 will stay where it is
			auto phase2Ranges = test::PrepareRanges(Disruptor_Size - 2);
			ProcessAll(*pDispatcher, std::move(phase2Ranges));
			WAIT_FOR_VALUE(offset * Disruptor_Size + 2 + 2 + 1, totalCounter);

			CATAPULT_LOG(info) << "all consumers reached their predefined positions";
			for (auto i = 0u; i < counters.size(); ++i)
				CATAPULT_LOG(debug) << "counters[" << i << "]: " << counters[i];

			// Act + Assert: adding another element to the disruptor will let processElement detect the failure
			//         processElement will attempt to process at (offset - 1 + Disruptor_Size) but inspector is only at (offset - 1)
			CATAPULT_LOG(info) << "consumer 0 is processing the deadly element";
			EXPECT_THROW(pDispatcher->processElement(ConsumerInput(test::CreateBlockEntityRange(1))), catapult_runtime_error);
		}
	}

	TEST(TEST_CLASS, DispatcherThrowsWhenDisruptorSpaceIsExhaustedEvenWhenOtherConsumersHaveNearbyPositions_NoWrapAround) {
		AssertDeathIfDisruptorSpaceIsExhaustedEvenIfOtherConsumersHaveNearbyPositions(1);
	}

	TEST(TEST_CLASS, DispatcherThrowsWhenDisruptorSpaceIsExhaustedEvenWhenOtherConsumersHaveNearbyPositions_WrapAround) {
		AssertDeathIfDisruptorSpaceIsExhaustedEvenIfOtherConsumersHaveNearbyPositions(10);
	}

	// endregion
}}
