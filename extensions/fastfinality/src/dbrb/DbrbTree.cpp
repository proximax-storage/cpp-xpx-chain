/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbTree.h"
#include "catapult/exceptions.h"
#include "catapult/dbrb/View.h"
#include <queue>

// DBRB shards are organized into a balanced tree. Each DBRB process is a part of 1 or 2 shards, parent shard and child shard.
// E.g. lets the shard size is 4, M is the DBRB process running on this node, B is the message broadcaster.
// Then here is possible tree of shards:
//                                                                     B
//                                                                    /|\
//                                                                /    |   \
//                                                             /       |      \
//                                                          /          |         \
//                                                       /             |            \
//                                                    /                |               \
//                                                 /                   |                  \
//                                              /                      |                     \
//                                            P-------------------------------------------------
//                                           /|\                      /|\                     /|\
//                                          / | \                    / | \                   / | \
//                                         /  |  \                  /  |  \                 /  |  \
//                                        /   |   \                /   |   \               /   |   \
//                                       /    |    \              /    |    \             /    |    \
//                                      /     |     \            /     |     \           /     |     \
//                                     /      |      \          /      |      \         /      |      \
//                                    /       |       \        /       |       \       /       |       \
//                                   S--------M--------S      -------------------     -------------------
//                                  /|\      /|\       |
//                                 / | \    / | \      |
//                                -------  C--C--C     D
//
// P is the parent process, S is a sibling process, C is a child process.
// P + S + S + M is the parent shard where M is a sibling.
// C + C + C + M is the child shard where M is a parent.
//
// Note: the last child shard may be not full, i.e. the number of DBRB processes in the shard may be less than the shard size.
// E.g P + D is a shard consisting of 2 DBRB processes.
//
// Nodes in each shard represent different vote powers:
// - each sibling has the power of the processes that constitute the subtree where the sibling is root
// - the parent process has the power of the rest of the entire tree

namespace catapult { namespace dbrb {

	namespace {
		struct DbrbTreeNode {
			size_t Index = 0;
			size_t MaxNodesToRemove = 0;
			size_t NodeCount = 0;
			size_t ChildIndex = 0;
			DbrbTreeNode* Parent = nullptr;
			std::vector<DbrbTreeNode*> Children;
		};

		std::vector<DbrbTreeNode> BuildDbrbTree(size_t nodeCount, size_t shardSize) {
			std::vector<DbrbTreeNode> tree(nodeCount);
			if (!nodeCount)
				return tree;

			tree[0] = DbrbTreeNode { 0, 0, 1, 0, nullptr, {} };
			auto pRoot = &tree[0];
			auto pParent = pRoot;
			auto childCount = shardSize - 1;
			size_t levelIndex = 0;
			size_t childIndex = 0;
			auto levelNodeCount = childCount;
			for (size_t index = 1; index < tree.size(); ++index) {
				tree[index] = DbrbTreeNode { index, 0, 1, childIndex, pParent, {} };
				pParent->Children.emplace_back(&tree[index]);
				auto pNode = pParent;
				while (pNode) {
					pNode->NodeCount++;
					pNode = pNode->Parent;
				}

				childIndex++;
				if (++levelIndex >= levelNodeCount) {
					pParent = pRoot;
					while (!pParent->Children.empty())
						pParent = pParent->Children[0];
					levelNodeCount *= childCount;
					levelIndex = 0;
					childIndex = 0;
				} else if (childIndex >= childCount) {
					auto currentLevelIndex = levelIndex;
					std::vector<size_t> parentIndexes {};
					while (currentLevelIndex >= childCount) {
						pParent = pParent->Parent;
						currentLevelIndex /= childCount;
						parentIndexes.emplace_back(currentLevelIndex % childCount);
					}
					for (auto iter = parentIndexes.rbegin(); iter != parentIndexes.rend(); ++iter)
						pParent = pParent->Children[*iter];
					childIndex = 0;
				}
			}

			return tree;
		}

		void AddProcessesToView(DbrbTreeView& view, ViewData::const_iterator& iter, const DbrbTreeNode* pTree, bool addRoot) {
			std::queue<DbrbTreeNode*> queue;
			if (addRoot)
				view[pTree->Index] = *iter++;

			for (const auto& pSubTree : pTree->Children) {
				if (pSubTree)
					queue.emplace(pSubTree);
			}

			while (!queue.empty()) {
				auto* pSubTree = queue.front();
				queue.pop();
				view[pSubTree->Index] = *iter++;
				for (auto* pChild : pSubTree->Children) {
					if (pChild)
						queue.emplace(pChild);
				}
			}
		}
	}

	DbrbTreeView CreateDbrbTreeView(const ViewData& reachableNodes, const ViewData& unreachableNodes, const ProcessId& broadcaster, size_t shardSize) {
		if (reachableNodes.find(broadcaster) != reachableNodes.cend())
			CATAPULT_THROW_INVALID_ARGUMENT("broadcaster is found in reachable nodes")

		if (unreachableNodes.find(broadcaster) != unreachableNodes.cend())
			CATAPULT_THROW_INVALID_ARGUMENT("broadcaster is found in unreachable nodes")

		auto unreachableNodeCount = unreachableNodes.size();
		auto nodeCount = reachableNodes.size() + unreachableNodeCount + 1;
		if (unreachableNodeCount > View::maxInvalidProcesses(nodeCount)) {
			CATAPULT_LOG(warning) << "[DBRB SHARD] too many unreachable nodes " << unreachableNodeCount << " in view (" << nodeCount << ")";
			return {};
		}

		DbrbTreeView view(nodeCount);
		view[0] = broadcaster;
		if (!unreachableNodeCount || nodeCount <= shardSize) {
			size_t index = 0;
			auto iter = reachableNodes.cbegin();
			for (; iter != reachableNodes.cend(); ++iter)
				view[++index] = *iter;

			iter = unreachableNodes.cbegin();
			for (; iter != unreachableNodes.cend(); ++iter)
				view[++index] = *iter;

			return view;
		}

		auto tree = BuildDbrbTree(nodeCount, shardSize);
		auto pRoot = &tree[0];
		pRoot->MaxNodesToRemove = View::maxInvalidProcesses(pRoot->NodeCount);
		std::queue<DbrbTreeNode*> queue;
		for (const auto& pSubTree : pRoot->Children)
			queue.emplace(pSubTree);

		std::vector<DbrbTreeNode*> removedSubtrees;
		removedSubtrees.reserve(unreachableNodeCount);
		while (unreachableNodeCount) {
			if (queue.empty())
				CATAPULT_THROW_RUNTIME_ERROR("failed to create DBRB tree")

			auto* pSubTree = queue.front();
			queue.pop();

			if (pSubTree->NodeCount <= unreachableNodeCount && pSubTree->NodeCount <= pSubTree->Parent->MaxNodesToRemove) {
				removedSubtrees.emplace_back(pSubTree);
				unreachableNodeCount -= pSubTree->NodeCount;
				pSubTree->Parent->Children[pSubTree->ChildIndex] = nullptr;
				auto pNodeToUpdate = pSubTree->Parent;
				while (pNodeToUpdate) {
					pNodeToUpdate->MaxNodesToRemove -= pSubTree->NodeCount;
					pNodeToUpdate = pNodeToUpdate->Parent;
				}
			} else {
				pSubTree->MaxNodesToRemove = View::maxInvalidProcesses(pSubTree->NodeCount);
				for (auto* pChild : pSubTree->Children)
					queue.emplace(pChild);
			}
		}

		auto iter = reachableNodes.cbegin();
		AddProcessesToView(view, iter, pRoot, false);

		iter = unreachableNodes.cbegin();
		for (const auto& pTree: removedSubtrees)
			AddProcessesToView(view, iter, pTree, true);

		return view;
	}

	DbrbDoubleShard CreateDbrbShard(const DbrbTreeView& view, const ProcessId& thisProcessId, size_t shardSize) {
		if (shardSize < DbrbDoubleShard::MinShardSize)
			CATAPULT_THROW_INVALID_ARGUMENT_2("shard size invalid", shardSize, DbrbDoubleShard::MinShardSize)

		if (view.empty()) {
			CATAPULT_LOG(error) << "failed to create DBRB shard, view is empty";
			return {};
		}

		DbrbDoubleShard shard;

		auto nodeCount = view.size();
		auto childCount = shardSize - 1;

		size_t index = 0;
		size_t levelIndex = 0;
		size_t levelNodeCount = 1;
		bool thisNodeNotFound = true;
		for (; index < nodeCount; ++index) {
			const auto& id = view[index];
			if (id == thisProcessId) {
				thisNodeNotFound = false;
				break;
			}

			shard.ParentView.emplace(id);

			if (++levelIndex >= levelNodeCount) {
				levelIndex = 0;
				levelNodeCount *= childCount;
			}
		}

		if (thisNodeNotFound) {
			CATAPULT_LOG(warning) << "[DBRB] TREE: this process " << thisProcessId << " not found in tree";
			return shard;
		}

		auto thisNodeLevelIndex = levelIndex;
		auto childIndex = levelIndex % childCount;
		auto parentLevelIndex = levelIndex / childCount;
		auto parentLevelNodeCount = levelNodeCount / childCount;
		std::map<size_t, ViewData&> siblingViews;
		if (levelNodeCount > 1) {
			auto levelStartIndex = index - levelIndex;
			shard.Parent = view[levelStartIndex - (parentLevelNodeCount - parentLevelIndex)];
			shard.Neighbours.emplace(shard.Parent);

			auto siblingsStartIndex = parentLevelIndex * childCount;
			for (auto i = 0; i < childCount; ++i) {
				if (i == childIndex)
					continue;

				auto siblingLevelIndex = siblingsStartIndex + i;
				auto siblingIndex = levelStartIndex + siblingLevelIndex;
				if (siblingIndex >= nodeCount)
					break;

				const auto& id = view[siblingIndex];
				shard.ParentView.erase(id);
				shard.Siblings.emplace(id);
				shard.Neighbours.emplace(id);
				shard.SiblingViews.emplace(id, ViewData{ id });
				siblingViews.emplace(siblingLevelIndex, shard.SiblingViews.at(id));
			}

			for (index += childCount - childIndex; index < levelStartIndex + levelNodeCount && index < nodeCount; ++index)
				shard.ParentView.emplace(view[index]);

			levelIndex = levelNodeCount - 1;
		} else {
			index++;
		}

		auto siblingsLevelNodeCount = levelNodeCount;
		auto childrenLevelNodeCount = siblingsLevelNodeCount * childCount;
		std::map<size_t, ViewData&> childViews;
		for (; index < nodeCount; ++index) {
			if (++levelIndex >= levelNodeCount) {
				levelIndex = 0;
				levelNodeCount *= childCount;
			}

			const auto& id = view[index];
			if (levelIndex * parentLevelNodeCount / levelNodeCount == parentLevelIndex) {
				auto siblingsLevelIndex = levelIndex * siblingsLevelNodeCount / levelNodeCount;
				if (siblingsLevelIndex == thisNodeLevelIndex) {
					auto childrenLevelIndex = levelIndex * childrenLevelNodeCount / levelNodeCount;
					auto iter = childViews.find(childrenLevelIndex);
					if (iter == childViews.cend()) {
						shard.Children.emplace(id);
						shard.Neighbours.emplace(id);
						shard.ChildViews.emplace(id, ViewData{ id });
						childViews.emplace(childrenLevelIndex, shard.ChildViews.at(id));
					} else {
						iter->second.emplace(id);
					}
				} else {
					siblingViews.at(siblingsLevelIndex).emplace(id);
				}
			} else {
				shard.ParentView.emplace(id);
			}
		}

		shard.Initialized = true;

		return shard;
	}
}}