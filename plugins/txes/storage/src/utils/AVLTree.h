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

class AVLTreeAdapter {
public:
	AVLTreeAdapter(
			cache::QueueCache::CacheDeltaType& queueCache,
			const Key& queueKey,
			std::function<state::AVLTreeNode (const Key&)> nodeExtractor,
			std::function<void (const Key&, const state::AVLTreeNode&)> nodeSaver)
		: m_queueCache(queueCache)
		, m_queueKey(queueKey)
		, m_nodeExtractor(std::move(nodeExtractor))
		, m_nodeSaver(std::move(nodeSaver))
	{}

	void insert(const Key& pValue) {
		setRoot(insert(getRoot(), pValue));
	}

	void remove(const Key& key) {
		setRoot(remove(getRoot(), key));
	}

	Key lowerBound(const Key& key) {
		return lowerBound(getRoot(), key);
	};

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

	int checkTreeValidity(const Key& nodeKey, bool& valid) {
		if (isNull(nodeKey))
			return 0;

		auto node = m_nodeExtractor(nodeKey);
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

	Key insert(const Key& nodeKey, const Key& insertedKey) {
		if (isNull(nodeKey))
			return insertedKey;

		state::AVLTreeNode node = m_nodeExtractor(nodeKey);

		if (insertedKey < nodeKey) {
			node.Left = insert(node.Left, insertedKey);
		}
		else {
			node.Right = insert(node.Right, insertedKey);
		}

		m_nodeSaver(nodeKey, node);

		updateHeight(nodeKey);
		return maybeRotate(nodeKey);
	}

	Key remove(const Key& nodeKey, const Key& removedKey) {
		if (isNull(nodeKey))
			return nodeKey;

		auto node = m_nodeExtractor(nodeKey);
		auto resultKey = nodeKey;

		if (nodeKey == removedKey) {
			if (isNull(node.Left) && isNull(node.Right)) {
				resultKey = {};
			}
			else if (isNull(node.Left)) {
				resultKey = node.Right;
			}
			else {
				auto [leftChild, replacer] = findReplacer(node.Left);
				auto rightChild = node.Right;
				resultKey = replacer;
				auto newNode = m_nodeExtractor(resultKey);
				newNode.Left = leftChild;
				newNode.Right = rightChild;
				m_nodeSaver(resultKey, newNode);
			}
		}
		else if (removedKey < nodeKey) {
			node.Left = remove(node.Left, removedKey);
			m_nodeSaver(nodeKey, node);
			updateHeight(nodeKey);
		}
		else {
			node.Right = remove(node.Right, removedKey);
			m_nodeSaver(nodeKey, node);
			updateHeight(nodeKey);
		}

		updateHeight(resultKey);
		return maybeRotate(resultKey);
	}

	// First element of the pair is standard return as in the insert case
	// Second element of the pair is the element that replaces the removed one
	std::pair<Key, Key> findReplacer(const Key& nodeKey) {
		// nodeKey is always not null
		auto node = m_nodeExtractor(nodeKey);

		if (isNull(node.Right)) {
			Key replacer = nodeKey;
			Key tmpParent = node.Left;
			updateHeight(tmpParent);
			return {maybeRotate(tmpParent), replacer};
		}
		else {
			auto replacer = findReplacer(node.Right);
			node.Right = replacer.first;
			m_nodeSaver(nodeKey, node);
			updateHeight(nodeKey);
			return {maybeRotate(nodeKey), replacer.second};
		}
	}

	Key lowerBound(const Key& nodeKey, const Key& key) {
		if (isNull(nodeKey))
			return nodeKey;

		auto node = m_nodeExtractor(nodeKey);

		if (nodeKey < key) {
			return lowerBound(node.Right, key);
		}
		else {
			Key leftLowerBound = lowerBound(node.Left, key);
			if (!isNull(leftLowerBound)) {
				return leftLowerBound;
			}
			else {
				return nodeKey;
			}
		}
	}

	uint16_t getHeight(const Key& nodeKey) const {
		if (isNull(nodeKey))
			return 0;

		return m_nodeExtractor(nodeKey).Height;
	}

	void updateHeight(const Key& nodeKey) {
		if (isNull(nodeKey))
			return;

		auto node = m_nodeExtractor(nodeKey);
		node.Height = std::max(getHeight(node.Left), getHeight(node.Right)) + 1;
		m_nodeSaver(nodeKey, node);
	}

	Rotation getRotationType(const Key& nodeKey) {
		if (isNull(nodeKey))
			return Rotation::NO_ROTATION;

		auto node = m_nodeExtractor(nodeKey);

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

	Key maybeRotate(const Key& nodeKey) {
		if (isNull(nodeKey))
			return nodeKey;

		auto rotation = getRotationType(nodeKey);

		switch (rotation) {
			case Rotation::LL_ROTATION: {
				return llRotation(nodeKey);
			}
			case Rotation::LR_ROTATION: {
				return lrRotation(nodeKey);
			}
			case Rotation::RL_ROTATION: {
				return rlRotation(nodeKey);
			}
			case Rotation::RR_ROTATION: {
				return rrRotation(nodeKey);
			}
		}
		return nodeKey;
	}

	Key llRotation(const Key& nodeKey) {
		auto node = m_nodeExtractor(nodeKey);

		Key parentKey = node.Left;
		auto parent = m_nodeExtractor(parentKey);

		node.Left = parent.Right;
		m_nodeSaver(nodeKey, node);
		updateHeight(nodeKey);

		parent.Right = nodeKey;
		m_nodeSaver(parentKey, parent);
		updateHeight(parentKey);

		return parentKey;
	}

	Key lrRotation(const Key& nodeKey) {
		auto node = m_nodeExtractor(nodeKey);
		auto left = m_nodeExtractor(node.Left);
		Key parentKey = left.Right;
		auto parent = m_nodeExtractor(parentKey);

		left.Right = parent.Left;
		m_nodeSaver(node.Left, left);
		updateHeight(node.Left);

		parent.Left = node.Left;

		node.Left = parent.Right;
		m_nodeSaver(nodeKey, node);
		updateHeight(nodeKey);

		parent.Right = nodeKey;
		m_nodeSaver(parentKey, parent);
		updateHeight(parentKey);

		return parentKey;
	}

	Key rlRotation(const Key& nodeKey) {
		auto node = m_nodeExtractor(nodeKey);
		auto right = m_nodeExtractor(node.Right);
		Key parentKey = right.Left;
		auto parent = m_nodeExtractor(parentKey);

		right.Left = parent.Right;
		m_nodeSaver(node.Right, right);
		updateHeight(node.Right);

		parent.Right = node.Right;

		node.Right = parent.Left;
		m_nodeSaver(nodeKey, node);
		updateHeight(nodeKey);

		parent.Left = nodeKey;
		m_nodeSaver(parentKey, parent);
		updateHeight(parentKey);

		return parentKey;
	}

	Key rrRotation(const Key& nodeKey) {
		auto node = m_nodeExtractor(nodeKey);
		Key parentKey = node.Right;
		auto parent = m_nodeExtractor(parentKey);

		node.Right = parent.Left;
		m_nodeSaver(nodeKey, node);
		updateHeight(nodeKey);

		parent.Left = nodeKey;
		m_nodeSaver(parentKey, parent);
		updateHeight(parentKey);

		return parentKey;
	}

	cache::QueueCache::CacheDeltaType& m_queueCache;
	Key m_queueKey;

	std::function<state::AVLTreeNode (const Key&)> m_nodeExtractor;
	std::function<void (const Key&, const state::AVLTreeNode&)> m_nodeSaver;
};
}