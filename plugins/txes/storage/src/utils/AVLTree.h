#pragma once

#include "src/state/QueueEntry.h"
#include "src/utils/StorageUtils.h"
#include "src/state/CommonEntities.h"

namespace catapult::utils {

using Pointer = Key;

enum class Rotation {
	NO_ROTATION,
	LL_ROTATION,
	LR_ROTATION,
	RR_ROTATION,
	RL_ROTATION
};

template<class TKeyType>
class AVLTreeAdapter {
public:
	AVLTreeAdapter(
			cache::QueueCache::CacheDeltaType& queueCache,
			const Key& queueKey,
			const std::function<TKeyType(const Pointer& pointer)>& keyExtractor,
			const std::function<state::AVLTreeNode& (const Pointer& pointer)>& nodeExtractor)
		: m_queueCache(queueCache)
		, m_queueKey(queueKey)
		, m_keyExtractor(keyExtractor)
		, m_nodeExtractor(nodeExtractor)
	{}

	void insert(const Pointer& pValue) {
		setRoot(insert(getRoot(), pValue));
	}

	void remove(const TKeyType& key) {
		setRoot(remove(getRoot(), key));
	}

	Pointer lowerBound(const TKeyType& key) {
		return lowerBound(getRoot(), key);
	};

	bool checkTreeValidity() {
		bool valid = true;
		int totalHeight = checkTreeValidity(getRoot(), valid);
		return valid;
	}

private:

	Pointer getRoot() {
		if (!m_queueCache.contains(m_queueKey)) {
			state::QueueEntry entry(m_queueKey);
			m_queueCache.insert(entry);
		}
		return m_queueCache.find(m_queueKey).get().getFirst();
	}

	int checkTreeValidity(const Pointer& pointer, bool& valid) {
		if (isNull(pointer)) {
			return 0;
		}
		const auto& node = m_nodeExtractor(pointer);
		auto leftHeight = checkTreeValidity(node.m_left, valid);
		auto rightHeight = checkTreeValidity(node.m_right, valid);
		if (abs(leftHeight - rightHeight) > 1) {
			valid = false;
		}
		return std::max(leftHeight, rightHeight) + 1;
	}

	Pointer setRoot(const Pointer& root) {
		if (!m_queueCache.contains(m_queueKey)) {
			state::QueueEntry entry(m_queueKey);
			m_queueCache.insert(entry);
		}
		m_queueCache.find(m_queueKey).get().setFirst(root);
	}

	bool isNull(const Pointer& pNode) const {
		return pNode == Pointer();
	}

	Pointer insert(const Pointer& pNode, const Pointer& pInsertedValue) {
		if (isNull(pNode)) {
			return pInsertedValue;
		}

		state::AVLTreeNode& node = m_nodeExtractor(pNode);
		const auto nodeKey = m_keyExtractor(pNode);

		const auto insertedKey = m_keyExtractor(pInsertedValue);

		if (insertedKey < nodeKey) {
			node.m_left = insert(node.m_left, pInsertedValue);
		}
		else {
			node.m_right = insert(node.m_right, pInsertedValue);
		}

		updateHeight(pNode);
		return maybeRotate(pNode);
	}

	Pointer remove(Pointer pNode, const TKeyType& removedKey) {
		if (isNull(pNode)) {
			return pNode;
		}

		auto& node = m_nodeExtractor(pNode);
		const auto nodeKey = m_keyExtractor(pNode);

		if (nodeKey == removedKey) {
			if (isNull(node.m_left) && isNull(node.m_right)) {
				pNode = {};
			}
			else if (isNull(node.m_left)) {
				pNode = node.m_right;
			}
			else {
				auto [leftChild, replacer] = findReplacer(node.m_left);
				auto rightChild = node.m_right;
				pNode = replacer;
				auto& newNode = m_nodeExtractor(pNode);
				newNode.m_left = leftChild;
				newNode.m_right = rightChild;
			}
		}
		else if (removedKey < nodeKey) {
			node.m_left = remove(node.m_left, removedKey);
		}
		else {
			node.m_right = remove(node.m_right, removedKey);
		}
		updateHeight(pNode);
		return maybeRotate(pNode);
	}

	// First element of the pair is standard return as in the insert case
	// Second element of the pair is the element that replaces the removed one
	std::pair<Pointer, Pointer> findReplacer(const Pointer& pNode) {

		// pNode is always not null
		auto& node = m_nodeExtractor(pNode);

		if (isNull(node.m_right)) {
			Pointer replacer = pNode;
			Pointer tmpParent = node.m_left;
			updateHeight(tmpParent);
			return {maybeRotate(tmpParent), replacer};
		}
		else {
			auto replacer = findReplacer(node.m_right);
			node.m_right = replacer.first;
			updateHeight(pNode);
			return {maybeRotate(pNode), replacer.second};
		}
	}

	Pointer lowerBound(const Pointer& pNode, const Key& key) {
		if (isNull(pNode)) {
			return pNode;
		}

		const auto nodeKey = m_keyExtractor(pNode);
		const auto& node = m_nodeExtractor(pNode);

		if (nodeKey < key) {
			return lowerBound(node.m_right, key);
		}
		else {
			Pointer leftLowerBound = lowerBound(node.m_left, key);
			if (!isNull(leftLowerBound)) {
				return leftLowerBound;
			}
			else {
				return pNode;
			}
		}
	}

	int getHeight(const Pointer& pNode) const {

		if (isNull(pNode)) {
			return 0;
		}

		const auto& node = m_nodeExtractor(pNode);
		return node.m_height;
	}

	void updateHeight(const Pointer& pNode) {
		if (isNull(pNode)) {
			return;
		}

		state::AVLTreeNode& node = m_nodeExtractor(pNode);

		node.m_height = std::max(getHeight(node.m_left), getHeight(node.m_right)) + 1;
	}

	Rotation getRotationType(const Pointer& pNode) {
		if (isNull(pNode)) {
			return Rotation::NO_ROTATION;
		}

		auto& node = m_nodeExtractor(pNode);

		auto leftHeight = getHeight(node.m_left);
		auto rightHeight = getHeight(node.m_right);

		if (abs(leftHeight - rightHeight) <= 1) {
			return Rotation::NO_ROTATION;
		}

		if (leftHeight > rightHeight) {
			// Left rotation is needed
			const auto& left = m_nodeExtractor(node.m_left);
			if (getHeight(left.m_left) >= getHeight(left.m_right)) {
				return Rotation::LL_ROTATION;
			}
			else {
				return Rotation::LR_ROTATION;
			}
		}
		else {
			// Right rotation is needed
			const auto& right = m_nodeExtractor(node.m_right);
			if (getHeight(right.m_right) >= getHeight(right.m_left)) {
				return Rotation::RR_ROTATION;
			}
			else {
				return Rotation::RL_ROTATION;
			}
		}
	}

	Pointer maybeRotate(const Pointer& pNode) {
		if (isNull(pNode)) {
			return pNode;
		}

		auto rotation = getRotationType(pNode);

		switch (rotation) {
			case Rotation::LL_ROTATION: {
				return llRotation(pNode);
			}
			case Rotation::LR_ROTATION: {
				return lrRotation(pNode);
			}
			case Rotation::RL_ROTATION: {
				return rlRotation(pNode);
			}
			case Rotation::RR_ROTATION: {
				return rrRotation(pNode);
			}
		}
		return pNode;
	}

	Pointer llRotation(const Pointer& pNode) {

		auto& node = m_nodeExtractor(pNode);

		Pointer pParent = node.m_left;
		auto& parent = m_nodeExtractor(pParent);

		node.m_left = parent.m_right;
		updateHeight(pNode);

		parent.m_right = pNode;
		updateHeight(pParent);

		return pParent;
	}

	Pointer lrRotation(const Pointer& pNode) {

		auto& node = m_nodeExtractor(pNode);

		auto& left = m_nodeExtractor(node.m_left);

		Pointer pParent = left.m_right;
		auto& parent = m_nodeExtractor(pParent);

		left.m_right = parent.m_left;
		updateHeight(node.m_left);

		parent.m_left = node.m_left;

		node.m_left = parent.m_right;
		updateHeight(pNode);

		parent.m_right = pNode;
		updateHeight(pParent);

		return pParent;
	}

	Pointer rlRotation(const Pointer& pNode) {

		auto& node = m_nodeExtractor(pNode);

		auto& right = m_nodeExtractor(node.m_right);

		Pointer pParent = right.m_left;
		auto& parent = m_nodeExtractor(pParent);

		right.m_left = parent.m_right;
		updateHeight(node.m_right);

		parent.m_right = node.m_right;

		node.m_right = parent.m_left;
		updateHeight(pNode);

		parent.m_left = pNode;
		updateHeight(pParent);

		return pParent;
	}

	Pointer rrRotation(const Pointer& pNode) {
		auto& node = m_nodeExtractor(pNode);

		Pointer pParent = node.m_right;
		auto& parent = m_nodeExtractor(pParent);

		node.m_right = parent.m_left;
		updateHeight(pNode);

		parent.m_left = pNode;
		updateHeight(pParent);

		return pParent;
	}

	cache::QueueCache::CacheDeltaType& m_queueCache;
	Key m_queueKey;

	std::function<const TKeyType (const Pointer& pointer)> m_keyExtractor;
	std::function<state::AVLTreeNode& (const Pointer& pointer)> m_nodeExtractor;
};
}