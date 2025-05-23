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

#include "RootNamespaceHistory.h"

namespace catapult { namespace state {

	namespace {
		void AddAllIds(std::set<NamespaceId>& ids, const RootNamespace::Children& children) {
			for (const auto& pair : children)
				ids.insert(pair.first);
		}

		void RemoveAllIds(std::set<NamespaceId>& ids, const RootNamespace::Children& children) {
			for (const auto& pair : children)
				ids.erase(pair.first);
		}
	}

	RootNamespaceHistory::RootNamespaceHistory(NamespaceId id) : m_id(id)
	{}

	RootNamespaceHistory::RootNamespaceHistory(const RootNamespaceHistory& history) : m_id(history.m_id) {
		std::shared_ptr<RootNamespace::Children> pChildren;
		auto owner = Key();
		for (const auto& root : history) {
			if (owner != root.owner()) {
				pChildren = std::make_shared<RootNamespace::Children>(root.children());
				owner = root.owner();
			}

			m_rootHistory.emplace_back(root.id(), root.owner(), root.lifetime(), pChildren);
			m_rootHistory.back().setAlias(root.id(), root.alias(root.id()));
		}
	}

	RootNamespaceHistory& RootNamespaceHistory::operator=(const RootNamespaceHistory& history) {
	    m_id = history.m_id;
		std::shared_ptr<RootNamespace::Children> pChildren;
		auto owner = Key();
		for (const auto& root : history) {
			if (owner != root.owner()) {
				pChildren = std::make_shared<RootNamespace::Children>(root.children());
				owner = root.owner();
			}

			m_rootHistory.emplace_back(root.id(), root.owner(), root.lifetime(), pChildren);
			m_rootHistory.back().setAlias(root.id(), root.alias(root.id()));
		}
		return *this;
	}

	bool RootNamespaceHistory::empty() const {
		return m_rootHistory.empty();
	}

	NamespaceId RootNamespaceHistory::id() const {
		return m_id;
	}

	size_t RootNamespaceHistory::historyDepth() const {
		return m_rootHistory.size();
	}

	size_t RootNamespaceHistory::activeOwnerHistoryDepth() const {
		if (m_rootHistory.empty())
			return 0;

		auto historyDepth = 0u;
		const auto& activeOwner = m_rootHistory.back().owner();
		for (auto iter = m_rootHistory.crbegin(); m_rootHistory.crend() != iter; ++iter) {
			if (activeOwner != iter->owner())
				break;

			++historyDepth;
		}

		return historyDepth;
	}

	size_t RootNamespaceHistory::numActiveRootChildren() const {
		return m_rootHistory.empty() ? 0 : back().size();
	}

	size_t RootNamespaceHistory::numAllHistoricalChildren() const {
		return utils::Sum(m_rootHistory, [](const auto& rootNamespace) { return rootNamespace.size(); });
	}

	void RootNamespaceHistory::push_back(const Key& owner, const NamespaceLifetime& lifetime, bool setAliasOnRenew) {
		if (!m_rootHistory.empty()) {
			const auto& previousNamespace = back();
			if (previousNamespace.owner() == owner) {
				// inherit all children since it is the same owner
				m_rootHistory.push_back(previousNamespace.renew(lifetime, setAliasOnRenew));
				return;
			}
		}

		m_rootHistory.emplace_back(m_id, owner, lifetime);
	}

	void RootNamespaceHistory::pop_back() {
		m_rootHistory.pop_back();
	}

	const RootNamespace& RootNamespaceHistory::back() const {
		return m_rootHistory.back();
	}

	RootNamespace& RootNamespaceHistory::back() {
		return m_rootHistory.back();
	}

	std::set<NamespaceId> RootNamespaceHistory::prune(Height height) {
		std::set<NamespaceId> ids;
		for (auto iter = m_rootHistory.begin(); m_rootHistory.end() != iter;) {
			if (iter->lifetime().GracePeriodEnd <= height) {
				AddAllIds(ids, iter->children());
				iter = m_rootHistory.erase(iter);
			} else {
				RemoveAllIds(ids, iter->children());
				++iter;
			}
		}

		if (m_rootHistory.empty())
			ids.insert(m_id);

		return ids;
	}

	std::list<RootNamespace>::const_iterator RootNamespaceHistory::begin() const {
		return m_rootHistory.cbegin();
	}

	std::list<RootNamespace>::const_iterator RootNamespaceHistory::end() const {
		return m_rootHistory.cend();
	}

	bool RootNamespaceHistory::isActiveAndUnlocked(Height height) const {
		return !empty() && back().lifetime().isActiveAndUnlocked(height);
	}

	bool RootNamespaceHistory::isActive(Height height) const {
		return !empty() && back().lifetime().isActiveOrGracePeriod(height);
	}
}}
