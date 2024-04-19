/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "fastfinality/src/dbrb/DbrbTree.h"
#include "catapult/dbrb/View.h"
#include "catapult/ionet/Node.h"
#include "tests/TestHarness.h"

namespace catapult { namespace dbrb {

#define TEST_CLASS DbrbTreeTests

	namespace {
		void RunTreeTest(uint8_t processCount, size_t shardSize, uint8_t unreachableNodeCount, const std::map<size_t, uint8_t>& unreachableNodeIndexes) {
			// Arrange:
			ProcessId broadcaster({ 1 });
			ViewData reachableNodes;
			for (uint8_t i = 2; i <= processCount - unreachableNodeCount; ++i)
				reachableNodes.emplace(ProcessId({ i }));
			ViewData unreachableNodes;
			for (uint8_t i = processCount - unreachableNodeCount + 1; i <= processCount; ++i)
				unreachableNodes.emplace(ProcessId({ i }));

			DbrbTreeView expectedView(processCount);
			expectedView[0] = broadcaster;
			for (const auto& pair : unreachableNodeIndexes)
				expectedView[pair.first] = ProcessId({ pair.second });
			auto iter = reachableNodes.cbegin();
			for (size_t index = 1; index < processCount; ++index) {
				if (unreachableNodeIndexes.find(index) == unreachableNodeIndexes.cend())
					expectedView[index] = *iter++;
			}

			// Act:
			auto view = CreateDbrbTreeView(reachableNodes, unreachableNodes, broadcaster, shardSize);

			// Assert:
			ASSERT_EQ(expectedView, view);
		}
	}

	TEST(TEST_CLASS, TreeView1) {
		RunTreeTest(6, 5, View::maxInvalidProcesses(6), {
			{ 2, 6 },
		});
	}

	TEST(TEST_CLASS, TreeView2) {
		RunTreeTest(44, 5, View::maxInvalidProcesses(44), {
			{  2, 31 },
			{  9, 32 },
			{ 10, 33 },
			{ 11, 34 },
			{ 12, 35 },
			{ 37, 36 },
			{ 38, 37 },
			{ 39, 38 },
			{ 40, 39 },
			{ 41, 40 },
			{ 42, 41 },
			{ 43, 42 },
			{ 13, 43 },
			{ 17, 44 },
		});
	}

	TEST(TEST_CLASS, TreeView3) {
		RunTreeTest(44, 5, 5, {
			{  3, 40 },
			{ 13, 41 },
			{ 14, 42 },
			{ 15, 43 },
			{ 16, 44 },
		});
	}

	TEST(TEST_CLASS, TreeView4) {
		RunTreeTest(7, 6, View::maxInvalidProcesses(7), {
			{ 1, 6 },
			{ 6, 7 },
		});
	}

	TEST(TEST_CLASS, TreeView5) {
		RunTreeTest(21, 6, 3, {
			{ 4, 19 },
			{ 5, 20 },
			{ 6, 21 },
		});
	}

	TEST(TEST_CLASS, TreeView6) {
		RunTreeTest(21, 6, View::maxInvalidProcesses(21), {
			{  1, 16 },
			{  6, 17 },
			{  7, 18 },
			{  8, 19 },
			{  9, 20 },
			{ 10, 21 },
		});
	}

	TEST(TEST_CLASS, TreeView7) {
		RunTreeTest(71, 6, View::maxInvalidProcesses(71), {
			{  2, 49 },
			{ 11, 50 },
			{ 12, 51 },
			{ 13, 52 },
			{ 14, 53 },
			{ 15, 54 },
			{ 56, 55 },
			{ 57, 56 },
			{ 58, 57 },
			{ 59, 58 },
			{ 60, 59 },
			{ 61, 60 },
			{ 62, 61 },
			{ 63, 62 },
			{ 64, 63 },
			{ 65, 64 },
			{ 66, 65 },
			{ 67, 66 },
			{ 68, 67 },
			{ 69, 68 },
			{ 70, 69 },
			{ 16, 70 },
			{ 21, 71 },
		});
	}

	TEST(TEST_CLASS, TreeView8) {
		RunTreeTest(100, 6, 0, {});
	}

	TEST(TEST_CLASS, TreeView9) {
		RunTreeTest(100, 6, 3, {
			{ 20,  98 },
			{ 21,  99 },
			{ 26, 100 },
		});
	}

	TEST(TEST_CLASS, TreeView11) {
		RunTreeTest(100, 6, 4, {
			{ 20,  97 },
			{ 21,  98 },
			{ 26,  99 },
			{ 31, 100 },
		});
	}

	TEST(TEST_CLASS, TreeView12) {
		RunTreeTest(100, 6, 5, {
			{ 19,  96 },
			{ 96,  97 },
			{ 97,  98 },
			{ 98,  99 },
			{ 99, 100 },
		});
	}

	TEST(TEST_CLASS, TreeView13) {
		RunTreeTest(100, 6, 10, {
			{  4,  91 },
			{ 21,  92 },
			{ 22,  93 },
			{ 23,  94 },
			{ 24,  95 },
			{ 25,  96 },
			{ 20,  97 },
			{ 26,  98 },
			{ 31,  99 },
			{ 36, 100 },
		});
	}

	TEST(TEST_CLASS, TreeView14) {
		RunTreeTest(100, 6, 20, {
			{  4,  81 },
			{ 21,  82 },
			{ 22,  83 },
			{ 23,  84 },
			{ 24,  85 },
			{ 25,  86 },
			{  5,  87 },
			{ 26,  88 },
			{ 27,  89 },
			{ 28,  90 },
			{ 29,  91 },
			{ 30,  92 },
			{  6,  93 },
			{ 31,  94 },
			{ 32,  95 },
			{ 33,  96 },
			{ 34,  97 },
			{ 35,  98 },
			{ 20,  99 },
			{ 36, 100 },
		});
	}

	TEST(TEST_CLASS, TreeView15) {
		RunTreeTest(100, 6, 30, {
			{  3,  71 },
			{ 16,  72 },
			{ 17,  73 },
			{ 18,  74 },
			{ 19,  75 },
			{ 20,  76 },
			{ 81,  77 },
			{ 82,  78 },
			{ 83,  79 },
			{ 84,  80 },
			{ 85,  81 },
			{ 86,  82 },
			{ 87,  83 },
			{ 88,  84 },
			{ 89,  85 },
			{ 90,  86 },
			{ 91,  87 },
			{ 92,  88 },
			{ 93,  89 },
			{ 94,  90 },
			{ 95,  91 },
			{ 96,  92 },
			{ 97,  93 },
			{ 98,  94 },
			{ 99,  95 },
			{ 21,  96 },
			{ 26,  97 },
			{ 31,  98 },
			{ 36,  99 },
			{ 41, 100 },
		});
	}

	TEST(TEST_CLASS, TreeView16) {
		RunTreeTest(100, 6, View::maxInvalidProcesses(100), {
			{  1,  68 },
			{  6,  69 },
			{  7,  70 },
			{  8,  71 },
			{  9,  72 },
			{ 10,  73 },
			{ 31,  74 },
			{ 32,  75 },
			{ 33,  76 },
			{ 34,  77 },
			{ 35,  78 },
			{ 36,  79 },
			{ 37,  80 },
			{ 38,  81 },
			{ 39,  82 },
			{ 40,  83 },
			{ 41,  84 },
			{ 42,  85 },
			{ 43,  86 },
			{ 44,  87 },
			{ 45,  88 },
			{ 46,  89 },
			{ 47,  90 },
			{ 48,  91 },
			{ 49,  92 },
			{ 50,  93 },
			{ 51,  94 },
			{ 52,  95 },
			{ 53,  96 },
			{ 54,  97 },
			{ 55,  98 },
			{ 20,  99 },
			{ 21, 100 },
		});
	}

	TEST(TEST_CLASS, TreeView17) {
		RunTreeTest(23, 6, 5, {
			{  4, 19 },
			{ 21, 20 },
			{ 22, 21 },
			{  5, 22 },
			{  6, 23 },
		});
	}

	TEST(TEST_CLASS, TreeView18) {
		RunTreeTest(85, 6, 18, {
			{  3, 68 },
			{ 16, 69 },
			{ 17, 70 },
			{ 18, 71 },
			{ 19, 72 },
			{ 20, 73 },
			{ 81, 74 },
			{ 82, 75 },
			{ 83, 76 },
			{ 84, 77 },
			{  4, 78 },
			{ 21, 79 },
			{ 22, 80 },
			{ 23, 81 },
			{ 24, 82 },
			{ 25, 83 },
			{ 26, 84 },
			{ 31, 85 },
		});
	}

	namespace {
		void RunShardTest(
				uint8_t processCount,
				size_t shardSize,
				uint8_t id,
				uint8_t expectedParentId,
				const std::set<uint8_t>& expectedSiblings,
				const std::set<uint8_t>& expectedChildren,
				const std::set<uint8_t>& expectedNeighbours,
				const std::set<uint8_t>& expectedParentView,
				const std::map<uint8_t, std::set<uint8_t>>& expectedSiblingViews,
				const std::map<uint8_t, std::set<uint8_t>>& expectedChildViews) {
			// Arrange:
			std::vector<ProcessId> view;
			for (uint8_t i = 1; i <= processCount; ++i)
				view.emplace_back(ProcessId({ i }));

			// Act:
			auto shard = CreateDbrbShard(view, ProcessId({ id }), shardSize);

			// Assert:
			EXPECT_EQ(ProcessId({ expectedParentId }), shard.Parent) << "expectedParentId = " << static_cast<int>(expectedParentId);

			ASSERT_EQ(expectedSiblings.size(), shard.Siblings.size());
			for (const auto& id : expectedSiblings)
				EXPECT_EQ(1, shard.Siblings.count(ProcessId({ id }))) << "id = " << static_cast<int>(id);

			ASSERT_EQ(expectedChildren.size(), shard.Children.size());
			for (const auto& id : expectedChildren)
				EXPECT_EQ(1, shard.Children.count(ProcessId({ id }))) << "id = " << static_cast<int>(id);

			ASSERT_EQ(expectedNeighbours.size(), shard.Neighbours.size());
			for (const auto& id : expectedNeighbours)
				EXPECT_EQ(1, shard.Neighbours.count(ProcessId({ id }))) << "id = " << static_cast<int>(id);

			ASSERT_EQ(expectedParentView.size(), shard.ParentView.size());
			for (const auto& id : expectedParentView)
				EXPECT_EQ(1, shard.ParentView.count(ProcessId({ id }))) << "id = " << static_cast<int>(id);

			ASSERT_EQ(expectedSiblingViews.size(), shard.SiblingViews.size());
			for (const auto& pair : expectedSiblingViews) {
				EXPECT_EQ(1, shard.SiblingViews.count(ProcessId({ pair.first }))) << "id = " << static_cast<int>(pair.first);
				const auto& view = shard.SiblingViews.at(ProcessId({ pair.first }));
				ASSERT_EQ(pair.second.size(), view.size());
				for (const auto& id : pair.second) {
					EXPECT_EQ(1, view.count(ProcessId({ id }))) << "id = " << static_cast<int>(id);
				}
			}

			ASSERT_EQ(expectedChildViews.size(), shard.ChildViews.size());
			for (const auto& pair : expectedChildViews) {
				EXPECT_EQ(1, shard.ChildViews.count(ProcessId({ pair.first }))) << "id = " << static_cast<int>(pair.first);
				const auto& view = shard.ChildViews.at(ProcessId({ pair.first }));
				ASSERT_EQ(pair.second.size(), view.size());
				for (const auto& id : pair.second) {
					EXPECT_EQ(1, view.count(ProcessId({ id }))) << "id = " << static_cast<int>(id);
				}
			}
		}
	}

	TEST(TEST_CLASS, Shard5_BroadcasterProcess) {
		RunShardTest(44, 5, 1, 0,
			{},
			{ 2, 3, 4, 5 },
			{ 2, 3, 4, 5 },
			{},
			std::map<uint8_t, std::set<uint8_t>>{},
			std::map<uint8_t, std::set<uint8_t>>{
				{ 2, std::set<uint8_t>{ 2, 6, 7, 8, 9, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37 } },
				{ 3, std::set<uint8_t>{ 3, 10, 11, 12, 13, 38, 39, 40, 41, 42, 43, 44 } },
				{ 4, std::set<uint8_t>{ 4, 14, 15, 16, 17 } },
				{ 5, std::set<uint8_t>{ 5, 18, 19, 20, 21 } },
			});
	}

	TEST(TEST_CLASS, Shard5_FirstProcess) {
		RunShardTest(44, 5, 2, 1,
			{ 3, 4, 5 },
			{ 6, 7, 8, 9 },
			{ 1, 3, 4, 5, 6, 7, 8, 9 },
			{ 1 },
			std::map<uint8_t, std::set<uint8_t>>{
				{ 3, std::set<uint8_t>{ 3, 10, 11, 12, 13, 38, 39, 40, 41, 42, 43, 44 } },
				{ 4, std::set<uint8_t>{ 4, 14, 15, 16, 17 } },
				{ 5, std::set<uint8_t>{ 5, 18, 19, 20, 21 } },
			},
			std::map<uint8_t, std::set<uint8_t>>{
				{ 6, std::set<uint8_t>{ 6, 22, 23, 24, 25 } },
				{ 7, std::set<uint8_t>{ 7, 26, 27, 28, 29 } },
				{ 8, std::set<uint8_t>{ 8, 30, 31, 32, 33 } },
				{ 9, std::set<uint8_t>{ 9, 34, 35, 36, 37 } },
			});
	}

	TEST(TEST_CLASS, Shard5_LeafProcess) {
		RunShardTest(44, 5, 16, 4,
			{ 14, 15, 17 },
			{},
			{ 4, 14, 15, 17 },
			{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44 },
			std::map<uint8_t, std::set<uint8_t>>{
				{ 14, std::set<uint8_t>{ 14 } },
				{ 15, std::set<uint8_t>{ 15 } },
				{ 17, std::set<uint8_t>{ 17 } },
			},
			std::map<uint8_t, std::set<uint8_t>>{});
	}

	TEST(TEST_CLASS, Shard5_IntermediateProcess) {
		RunShardTest(44, 5, 7, 2,
			{ 6, 8, 9 },
			{ 26, 27, 28, 29 },
			{ 2, 6, 8, 9, 26, 27, 28, 29 },
			{ 1, 2, 3, 4, 5, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 38, 39, 40, 41, 42, 43, 44 },
			std::map<uint8_t, std::set<uint8_t>>{
				{ 6, std::set<uint8_t>{ 6, 22, 23, 24, 25 } },
				{ 8, std::set<uint8_t>{ 8, 30, 31, 32, 33 } },
				{ 9, std::set<uint8_t>{ 9, 34, 35, 36, 37 } },
			},
			std::map<uint8_t, std::set<uint8_t>>{
				{ 26, std::set<uint8_t>{ 26 } },
				{ 27, std::set<uint8_t>{ 27 } },
				{ 28, std::set<uint8_t>{ 28 } },
				{ 29, std::set<uint8_t>{ 29 } },
			});
	}

	TEST(TEST_CLASS, Shard5_LastProcess) {
		RunShardTest(44, 5, 44, 11,
			{ 42, 43 },
			{},
			{ 11, 42, 43 },
			{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41 },
			std::map<uint8_t, std::set<uint8_t>>{
				{ 42, std::set<uint8_t>{ 42 } },
				{ 43, std::set<uint8_t>{ 43 } },
			},
			std::map<uint8_t, std::set<uint8_t>>{});
	}

	TEST(TEST_CLASS, Shard6_BroadcasterProcess) {
		RunShardTest(44, 6, 1, 0,
			{},
			{ 2, 3, 4, 5, 6 },
			{ 2, 3, 4, 5, 6 },
			{},
			std::map<uint8_t, std::set<uint8_t>>{},
			std::map<uint8_t, std::set<uint8_t>>{
				{ 2, std::set<uint8_t>{ 2, 7, 8, 9, 10, 11, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44 } },
				{ 3, std::set<uint8_t>{ 3, 12, 13, 14, 15, 16 } },
				{ 4, std::set<uint8_t>{ 4, 17, 18, 19, 20, 21 } },
				{ 5, std::set<uint8_t>{ 5, 22, 23, 24, 25, 26 } },
				{ 6, std::set<uint8_t>{ 6, 27, 28, 29, 30, 31 } },
			});
	}

	TEST(TEST_CLASS, Shard6_FirstProcess) {
		RunShardTest(55, 6, 2, 1,
			{ 3, 4, 5, 6 },
			{ 7, 8, 9, 10, 11 },
			{ 1, 3, 4, 5, 6, 7, 8, 9, 10, 11 },
			{ 1 },
			std::map<uint8_t, std::set<uint8_t>>{
				{ 3, std::set<uint8_t>{ 3, 12, 13, 14, 15, 16 } },
				{ 4, std::set<uint8_t>{ 4, 17, 18, 19, 20, 21 } },
				{ 5, std::set<uint8_t>{ 5, 22, 23, 24, 25, 26 } },
				{ 6, std::set<uint8_t>{ 6, 27, 28, 29, 30, 31 } },
			},
			std::map<uint8_t, std::set<uint8_t>>{
				{ 7, std::set<uint8_t>{ 7, 32, 33, 34, 35, 36 } },
				{ 8, std::set<uint8_t>{ 8, 37, 38, 39, 40, 41 } },
				{ 9, std::set<uint8_t>{ 9, 42, 43, 44, 45, 46 } },
				{ 10, std::set<uint8_t>{ 10, 47, 48, 49, 50, 51 } },
				{ 11, std::set<uint8_t>{ 11, 52, 53, 54, 55 } },
			});
	}

	TEST(TEST_CLASS, Shard6_LeafProcess) {
		std::set<uint8_t> parentView;
		for (uint8_t i = 1; i <= 66; ++i) {
			if (i < 22 || i > 26)
				parentView.emplace(i);
		}

		RunShardTest(66, 6, 23, 5,
			{ 22, 24, 25, 26 },
			{},
			{ 5, 22, 24, 25, 26 },
			parentView,
			std::map<uint8_t, std::set<uint8_t>>{
				{ 22, std::set<uint8_t>{ 22 } },
				{ 24, std::set<uint8_t>{ 24 } },
				{ 25, std::set<uint8_t>{ 25 } },
				{ 26, std::set<uint8_t>{ 26 } },
			},
			std::map<uint8_t, std::set<uint8_t>>{});
	}

	TEST(TEST_CLASS, Shard6_IntermediateProcess) {
		RunShardTest(77, 6, 12, 3,
			{ 13, 14, 15, 16 },
			{ 57, 58, 59, 60, 61 },
			{ 3, 13, 14, 15, 16, 57, 58, 59, 60, 61 },
			{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56 },
			std::map<uint8_t, std::set<uint8_t>>{
				{ 13, std::set<uint8_t>{ 13, 62, 63, 64, 65, 66 } },
				{ 14, std::set<uint8_t>{ 14, 67, 68, 69, 70, 71 } },
				{ 15, std::set<uint8_t>{ 15, 72, 73, 74, 75, 76 } },
				{ 16, std::set<uint8_t>{ 16, 77 } },
			},
			std::map<uint8_t, std::set<uint8_t>>{
				{ 57, std::set<uint8_t>{ 57 } },
				{ 58, std::set<uint8_t>{ 58 } },
				{ 59, std::set<uint8_t>{ 59 } },
				{ 60, std::set<uint8_t>{ 60 } },
				{ 61, std::set<uint8_t>{ 61 } },
			});
	}

	TEST(TEST_CLASS, Shard6_LastProcess) {
		std::set<uint8_t> parentView;
		for (uint8_t i = 1; i <= 86; ++i)
			parentView.emplace(i);

		RunShardTest(88, 6, 88, 18,
			{ 87 },
			{},
			{ 18, 87 },
			parentView,
			std::map<uint8_t, std::set<uint8_t>>{
				{ 87, std::set<uint8_t>{ 87 } },
			},
			std::map<uint8_t, std::set<uint8_t>>{});
	}

	TEST(TEST_CLASS, Shard10_FewProcesses) {
		RunShardTest(4, 10, 3, 1,
			{ 2, 4 },
			{},
			{ 1, 2, 4 },
			{ 1 },
			std::map<uint8_t, std::set<uint8_t>>{
				{ 2, std::set<uint8_t>{ 2 } },
				{ 4, std::set<uint8_t>{ 4 } },
			},
			std::map<uint8_t, std::set<uint8_t>>{});
	}
}}