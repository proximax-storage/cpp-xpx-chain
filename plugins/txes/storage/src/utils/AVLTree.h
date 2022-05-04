/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/QueueEntry.h"
#include "src/utils/StorageUtils.h"
#include "src/state/CommonEntities.h"
#include <utility>

namespace catapult::utils {

enum class Rotation {
	NO_ROTATION,
	LL_ROTATION,
	LR_ROTATION,
	RR_ROTATION,
	RL_ROTATION
};

template <class TKey>
class AVLTreeAdapter {
public:
	AVLTreeAdapter(
			cache::QueueCache::CacheDeltaType& queueCache,
			const Key& queueKey,
			std::function<TKey (const Key&)> keyExtractor,
			std::function<state::AVLTreeNode (const Key&)> nodeExtractor,
			std::function<void (const Key&, const state::AVLTreeNode&)> nodeSaver)
		: m_queueCache(queueCache)
		, m_queueKey(queueKey)
		, m_keyExtractor(std::move(keyExtractor))
		, m_nodeExtractor(std::move(nodeExtractor))
		, m_nodeSaver(std::move(nodeSaver))
	{}

	void insert(const Key& pValue) {
		setRoot(insert(getRoot(), pValue));
	}

	void remove(const TKey& key) {
		setRoot(remove(getRoot(), key));
	}

	Key lowerBound(const TKey& key) {
		return lowerBound(getRoot(), key);
	};

	uint32_t numberOfLess(const TKey& key) {
		return numberOfLess(getRoot(), key);
	}

	Key extract(uint32_t index) {
		Key extractedPointer;
		setRoot(extract(getRoot(), index, extractedPointer));
		return extractedPointer;
	}

	bool checkTreeValidity() {
		bool valid = true;
		int totalHeight = checkTreeValidity(getRoot(), valid);
		return valid;
	}

private:

	Key getRoot() {
		if (!m_queueCache.contains(m_queueKey)) {
			state::QueueEntry entry(m_queueKey);
			m_queueCache.insert(entry);
		}
		return m_queueCache.find(m_queueKey).get().getFirst();
	}

	int checkTreeValidity(const Key& nodePointer, bool& valid) {
		if (isNull(nodePointer))
			return 0;

		auto node = m_nodeExtractor(nodePointer);
		auto leftHeight = checkTreeValidity(node.Left, valid);
		auto rightHeight = checkTreeValidity(node.Right, valid);
		if (abs(leftHeight - rightHeight) > 1) {
			valid = false;
		}
		return std::max(leftHeight, rightHeight) + 1;
	}

	Key setRoot(const Key& root) {
		if (!m_queueCache.contains(m_queueKey)) {
			state::QueueEntry entry(m_queueKey);
			m_queueCache.insert(entry);
		}
		m_queueCache.find(m_queueKey).get().setFirst(root);
	}

	bool isNull(const Key& nodeKey) const {
		return nodeKey == Key();
	}

	Key insert(const Key& nodePointer, const Key& insertedNodePointer) {
		if (isNull(nodePointer))
			return insertedNodePointer;

		state::AVLTreeNode node = m_nodeExtractor(nodePointer);

		auto nodeKey = m_keyExtractor(nodePointer);
		auto insertedKey = m_keyExtractor(insertedNodePointer);

		if (insertedKey < nodeKey) {
			node.Left = insert(node.Left, insertedNodePointer);
		}
		else {
			node.Right = insert(node.Right, insertedNodePointer);
		}

		m_nodeSaver(nodePointer, node);

		updateStatistics(nodePointer);
		return maybeRotate(nodePointer);
	}

	Key extract(const Key& nodePointer, int index, Key& extractedPointer) {
		if (isNull(nodePointer))
			return nodePointer;

		auto node = m_nodeExtractor(nodePointer);
		auto resultPointer = nodePointer;

		auto nodeKey = m_keyExtractor(nodePointer);

		auto leftSize = getSize(node.Left);

		if (index < leftSize) {
			node.Left = extract(node.Left, index, extractedPointer);
			m_nodeSaver(nodePointer, node);
			updateStatistics(nodePointer);
		}
		if (index == leftSize) {
			if (isNull(node.Left) && isNull(node.Right)) {
				resultPointer = {};
			}
			else if (isNull(node.Left)) {
				resultPointer = node.Right;
			}
			else {
				auto [leftChild, replacer] = findReplacer(node.Left);
				auto rightChild = node.Right;
				resultPointer = replacer;
				auto newNode = m_nodeExtractor(resultPointer);
				newNode.Left = leftChild;
				newNode.Right = rightChild;
				m_nodeSaver(resultPointer, newNode);
			}
			m_nodeSaver(nodePointer, state::AVLTreeNode());
			extractedPointer = nodePointer;
		}
		else {
			node.Right = extract(node.Right, index - leftSize - 1, extractedPointer);
			m_nodeSaver(nodePointer, node);
			updateStatistics(nodePointer);
		}

		updateStatistics(resultPointer);
		return maybeRotate(resultPointer);
	}

	Key remove(const Key& nodePointer, const TKey& removedKey) {
		if (isNull(nodePointer))
			return nodePointer;

		auto node = m_nodeExtractor(nodePointer);
		auto resultPointer = nodePointer;

		auto nodeKey = m_keyExtractor(nodePointer);
		if (nodeKey == removedKey) {
			if (isNull(node.Left) && isNull(node.Right)) {
				resultPointer = {};
			}
			else if (isNull(node.Left)) {
				resultPointer = node.Right;
			}
			else {
				auto [leftChild, replacer] = findReplacer(node.Left);
				auto rightChild = node.Right;
				resultPointer = replacer;
				auto newNode = m_nodeExtractor(resultPointer);
				newNode.Left = leftChild;
				newNode.Right = rightChild;
				m_nodeSaver(resultPointer, newNode);
			}
		}
		else if (removedKey < nodeKey) {
			node.Left = remove(node.Left, removedKey);
			m_nodeSaver(nodePointer, node);
			updateStatistics(nodePointer);
		}
		else {
			node.Right = remove(node.Right, removedKey);
			m_nodeSaver(nodePointer, node);
			updateStatistics(nodePointer);
		}

		updateStatistics(resultPointer);
		return maybeRotate(resultPointer);
	}

	// First element of the pair is standard return as in the insert case
	// Second element of the pair is the element that replaces the removed one
	std::pair<Key, Key> findReplacer(const Key& nodePointer) {
		// nodeKey is always not null
		auto node = m_nodeExtractor(nodePointer);

		if (isNull(node.Right)) {
			Key replacer = nodePointer;
			Key tmpParent = node.Left;
			updateStatistics(tmpParent);
			return {maybeRotate(tmpParent), replacer};
		}
		else {
			auto replacer = findReplacer(node.Right);
			node.Right = replacer.first;
			m_nodeSaver(nodePointer, node);
			updateStatistics(nodePointer);
			return {maybeRotate(nodePointer), replacer.second};
		}
	}

	Key lowerBound(const Key& nodePointer, const TKey& key) {
		if (isNull(nodePointer))
			return nodePointer;

		auto node = m_nodeExtractor(nodePointer);
		auto nodeKey = m_keyExtractor(nodePointer);

		if (nodeKey < key) {
			return lowerBound(node.Right, key);
		}
		else {
			Key leftLowerBound = lowerBound(node.Left, key);
			if (!isNull(leftLowerBound)) {
				return leftLowerBound;
			}
			else {
				return nodePointer;
			}
		}
	}

	uint32_t numberOfLess(const Key& nodePointer, const TKey& key) {
		if (isNull(nodePointer)) {
			return 0;
		}

		auto node = m_nodeExtractor(nodePointer);
		auto nodeKey = m_keyExtractor(nodePointer);

		if (nodeKey < key) {
			auto lefSize = getSize(node.Left);
			auto rightLess = numberOfLess(node.Right, key);
			return numberOfLess(node.Right, key) + getSize(node.Left) + 1;
		}

		return numberOfLess(node.Left, key);
	}

	uint16_t getHeight(const Key& nodePointer) const {
		if (isNull(nodePointer))
			return 0;

		return m_nodeExtractor(nodePointer).Height;
	}

	uint32_t getSize(const Key& nodePointer) const {
		if (isNull(nodePointer))
			return 0;

		return m_nodeExtractor(nodePointer).Size;
	}

	std::pair<uint16_t, uint32_t> getStatistics(const Key& nodePointer) {
		if (isNull(nodePointer)) {
			return {0, 0};
		}

		auto node = m_nodeExtractor(nodePointer);
		return {node.Height, node.Size};
	}

	void updateStatistics(const Key& nodePointer) {
		if (isNull(nodePointer))
			return;

		auto node = m_nodeExtractor(nodePointer);

		auto [leftHeight, leftSize] = getStatistics(node.Left);
		auto [rightHeight, rightSize] = getStatistics(node.Right);

		node.Height = std::max(leftHeight, rightHeight) + 1;
		node.Size = leftSize + rightSize + 1;
		m_nodeSaver(nodePointer, node);
	}

	Rotation getRotationType(const Key& nodePointer) {
		if (isNull(nodePointer))
			return Rotation::NO_ROTATION;

		auto node = m_nodeExtractor(nodePointer);

		auto leftHeight = getHeight(node.Left);
		auto rightHeight = getHeight(node.Right);

		if (abs(leftHeight - rightHeight) <= 1) {
			return Rotation::NO_ROTATION;
		}

		if (leftHeight > rightHeight) {
			// Left rotation is needed
			const auto& left = m_nodeExtractor(node.Left);
			if (getHeight(left.Left) >= getHeight(left.Right)) {
				return Rotation::LL_ROTATION;
			}
			else {
				return Rotation::LR_ROTATION;
			}
		}
		else {
			// Right rotation is needed
			const auto& right = m_nodeExtractor(node.Right);
			if (getHeight(right.Right) >= getHeight(right.Left)) {
				return Rotation::RR_ROTATION;
			}
			else {
				return Rotation::RL_ROTATION;
			}
		}
	}

	Key maybeRotate(const Key& nodePointer) {
		if (isNull(nodePointer))
			return nodePointer;

		auto rotation = getRotationType(nodePointer);

		switch (rotation) {
			case Rotation::LL_ROTATION: {
				return llRotation(nodePointer);
			}
			case Rotation::LR_ROTATION: {
				return lrRotation(nodePointer);
			}
			case Rotation::RL_ROTATION: {
				return rlRotation(nodePointer);
			}
			case Rotation::RR_ROTATION: {
				return rrRotation(nodePointer);
			}
		}
		return nodePointer;
	}

	Key llRotation(const Key& nodePointer) {
		auto node = m_nodeExtractor(nodePointer);

		Key parentPointer = node.Left;
		auto parent = m_nodeExtractor(parentPointer);

		node.Left = parent.Right;
		m_nodeSaver(nodePointer, node);
		updateStatistics(nodePointer);

		parent.Right = nodePointer;
		m_nodeSaver(parentPointer, parent);
		updateStatistics(parentPointer);

		return parentPointer;
	}

	Key lrRotation(const Key& nodePointer) {
		auto node = m_nodeExtractor(nodePointer);
		auto left = m_nodeExtractor(node.Left);
		Key parentPointer = left.Right;
		auto parent = m_nodeExtractor(parentPointer);

		left.Right = parent.Left;
		m_nodeSaver(node.Left, left);
		updateStatistics(node.Left);

		parent.Left = node.Left;

		node.Left = parent.Right;
		m_nodeSaver(nodePointer, node);
		updateStatistics(nodePointer);

		parent.Right = nodePointer;
		m_nodeSaver(parentPointer, parent);
		updateStatistics(parentPointer);

		return parentPointer;
	}

	Key rlRotation(const Key& nodePointer) {
		auto node = m_nodeExtractor(nodePointer);
		auto right = m_nodeExtractor(node.Right);
		Key parentPointer = right.Left;
		auto parent = m_nodeExtractor(parentPointer);

		right.Left = parent.Right;
		m_nodeSaver(node.Right, right);
		updateStatistics(node.Right);

		parent.Right = node.Right;

		node.Right = parent.Left;
		m_nodeSaver(nodePointer, node);
		updateStatistics(nodePointer);

		parent.Left = nodePointer;
		m_nodeSaver(parentPointer, parent);
		updateStatistics(parentPointer);

		return parentPointer;
	}

	Key rrRotation(const Key& nodePointer) {
		auto node = m_nodeExtractor(nodePointer);
		Key parentPointer = node.Right;
		auto parent = m_nodeExtractor(parentPointer);

		node.Right = parent.Left;
		m_nodeSaver(nodePointer, node);
		updateStatistics(nodePointer);

		parent.Left = nodePointer;
		m_nodeSaver(parentPointer, parent);
		updateStatistics(parentPointer);

		return parentPointer;
	}

	cache::QueueCache::CacheDeltaType& m_queueCache;
	Key m_queueKey;

	std::function<TKey (const Key&)> m_keyExtractor;
	std::function<state::AVLTreeNode (const Key&)> m_nodeExtractor;
	std::function<void (const Key&, const state::AVLTreeNode&)> m_nodeSaver;
};
}