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

#include "NamespaceCacheDelta.h"
#include "src/config/NamespaceConfiguration.h"
#include "catapult/cache/IdentifierGroupCacheUtils.h"
#include "catapult/plugins/PluginUtils.h"
#include "catapult/utils/Casting.h"
#include <numeric>
#include <unordered_set>

namespace catapult { namespace cache {

	namespace {
		using NamespaceByIdMap = NamespaceCacheTypes::FlatMapTypes::BaseSetDeltaType;

		void AddAll(NamespaceByIdMap& namespaceById, const state::RootNamespace::Children& children) {
			for (const auto& pair : children)
				namespaceById.insert(state::Namespace(pair.second.Path));
		}

		void RemoveAll(NamespaceByIdMap& namespaceById, const state::RootNamespace::Children& children) {
			for (const auto& pair : children)
				namespaceById.remove(pair.first);
		}

		constexpr auto AliasFixVersion = catapult::BlockchainVersion{
			uint64_t{1}	<< 48 |
			uint64_t{2}	<< 32 |
			uint64_t{0}	<< 16 |
			uint64_t{0}
		};
	}

	BasicNamespaceCacheDelta::BasicNamespaceCacheDelta(
				const NamespaceCacheTypes::BaseSetDeltaPointers& namespaceSets,
				const NamespaceCacheTypes::Options& options,
				const NamespaceSizes& namespaceSizes)
			: NamespaceCacheDeltaMixins::Size(*namespaceSets.pPrimary)
			, NamespaceCacheDeltaMixins::Contains(*namespaceSets.pFlatMap)
			, NamespaceCacheDeltaMixins::PatriciaTreeDelta(*namespaceSets.pPrimary, namespaceSets.pPatriciaTree)
			, NamespaceCacheDeltaMixins::Touch(*namespaceSets.pPrimary, *namespaceSets.pHeightGrouping)
			, NamespaceCacheDeltaMixins::DeltaElements(*namespaceSets.pPrimary)
			, NamespaceCacheDeltaMixins::NamespaceDeepSize(namespaceSizes)
			, NamespaceCacheDeltaMixins::NamespaceLookup(*namespaceSets.pPrimary, *namespaceSets.pFlatMap)
			, m_pHistoryById(namespaceSets.pPrimary)
			, m_pNamespaceById(namespaceSets.pFlatMap)
			, m_pRootNamespaceIdsByExpiryHeight(namespaceSets.pHeightGrouping)
			, m_pConfigHolder(options.ConfigHolderPtr)
	{}

	void BasicNamespaceCacheDelta::insert(const state::RootNamespace& ns) {
		// register the namespace for expiration at the end of its lifetime (if its lifetime changes later, it will not be pruned)
		const auto& blockchainConfig = m_pConfigHolder->Config(height());
		const auto& pluginConfig = blockchainConfig.Network.template GetPluginConfiguration<config::NamespaceConfiguration>();
		auto gracePeriodDuration = pluginConfig.NamespaceGracePeriodDuration.blocks(blockchainConfig.Network.BlockGenerationTargetTime);
		auto nsLifetimeWithGracePeriod = state::NamespaceLifetime(ns.lifetime().Start, ns.lifetime().End, gracePeriodDuration);
		AddIdentifierWithGroup(*m_pRootNamespaceIdsByExpiryHeight, nsLifetimeWithGracePeriod.GracePeriodEnd, ns.id());

		bool setAliasOnRenew = (m_pConfigHolder->Version(height()) >= AliasFixVersion);
		auto historyIter = m_pHistoryById->find(ns.id());
		auto* pHistory = historyIter.get();
		if (pHistory) {
			// if the owner changed, remove all of the current root's children
			const auto& activeChildren = pHistory->back().children();
			if (pHistory->back().owner() != ns.owner()) {
				RemoveAll(*m_pNamespaceById, activeChildren);
				decrementActiveSize(activeChildren.size());
			} else {
				incrementDeepSize(activeChildren.size());
			}

			pHistory->push_back(ns.owner(), nsLifetimeWithGracePeriod, setAliasOnRenew);
			incrementDeepSize();
			return;
		}

		state::RootNamespaceHistory history(ns.id());
		history.push_back(ns.owner(), nsLifetimeWithGracePeriod, setAliasOnRenew);
		m_pHistoryById->insert(std::move(history));
		incrementActiveSize();
		incrementDeepSize();

		state::Namespace::Path path;
		path.push_back(ns.id());
		m_pNamespaceById->insert(state::Namespace(path));
	}

	void BasicNamespaceCacheDelta::insert(const state::Namespace& ns) {
		auto historyIter = m_pHistoryById->find(ns.rootId());
		auto* pHistory = historyIter.get();
		if (!pHistory)
			CATAPULT_THROW_INVALID_ARGUMENT_1("no root namespace exists for namespace", ns.id());

		pHistory->back().add(ns);
		incrementActiveSize();
		incrementDeepSize(pHistory->activeOwnerHistoryDepth());

		m_pNamespaceById->insert(ns);
	}

	void BasicNamespaceCacheDelta::setAlias(NamespaceId id, const state::NamespaceAlias& alias) {
		auto namespaceIter = m_pNamespaceById->find(id);
		const auto* pNamespace = namespaceIter.get();
		if (!pNamespace)
			CATAPULT_THROW_INVALID_ARGUMENT_1("no namespace exists", id);

		// if child is known, root must be present
		auto historyIter = m_pHistoryById->find(pNamespace->rootId());
		historyIter.get()->back().setAlias(id, alias);
	}

	void BasicNamespaceCacheDelta::remove(NamespaceId id) {
		auto namespaceIter = m_pNamespaceById->find(id);
		const auto* pNamespace = namespaceIter.get();
		if (!pNamespace)
			CATAPULT_THROW_INVALID_ARGUMENT_1("no namespace exists", id);

		if (pNamespace->rootId() == id)
			removeRoot(id);
		else
			removeChild(*pNamespace);
	}

	void BasicNamespaceCacheDelta::removeRoot(NamespaceId id) {
		auto historyIter = m_pHistoryById->find(id);
		auto* pHistory = historyIter.get();
		if (1 == pHistory->historyDepth() && !pHistory->back().empty())
			CATAPULT_THROW_RUNTIME_ERROR_1("cannot remove non-empty root namespace", id);

		// make a copy of the current root and remove it
		auto removedRoot = pHistory->back();
		pHistory->pop_back();

		// remove the height based entry
		RemoveIdentifierWithGroup(*m_pRootNamespaceIdsByExpiryHeight, removedRoot.lifetime().GracePeriodEnd, id);

		if (pHistory->empty()) {
			// note that the last root in the history is always empty when getting removed
			m_pHistoryById->remove(id);
			m_pNamespaceById->remove(id);

			decrementActiveSize(1);
			decrementDeepSize(1);
			return;
		}

		// if the owner changed, update all children so that only the new root's children are contained
		const auto& currentRoot = pHistory->back();
		auto numRemovedChildren = removedRoot.children().size();
		auto numRemovedChildrenAndRoot = 1 + numRemovedChildren;
		if (removedRoot.owner() != currentRoot.owner()) {
			RemoveAll(*m_pNamespaceById, removedRoot.children());
			decrementActiveSize(numRemovedChildren);

			AddAll(*m_pNamespaceById, currentRoot.children());
			incrementActiveSize(currentRoot.children().size());
		}

		decrementDeepSize(numRemovedChildrenAndRoot);
	}

	void BasicNamespaceCacheDelta::removeChild(const state::Namespace& ns) {
		auto historyIter = m_pHistoryById->find(ns.rootId());
		auto* pHistory = historyIter.get();
		pHistory->back().remove(ns.id());
		m_pNamespaceById->remove(ns.id());

		decrementActiveSize();
		decrementDeepSize(pHistory->activeOwnerHistoryDepth());
	}

	namespace {
		NamespaceSizes GetNamespaceSizes(const state::RootNamespaceHistory& history) {
			return {
				(history.empty() ? 0 : 1) + history.numActiveRootChildren(),
				history.historyDepth() + history.numAllHistoricalChildren()
			};
		}
	}

	BasicNamespaceCacheDelta::CollectedIds BasicNamespaceCacheDelta::prune(Height height) {
		BasicNamespaceCacheDelta::CollectedIds collectedIds;
		ForEachIdentifierWithGroup(
				*m_pHistoryById,
				*m_pRootNamespaceIdsByExpiryHeight,
				height,
				[this, height, &collectedIds](auto& history) {
			auto originalSizes = GetNamespaceSizes(history);
			auto removedIds = history.prune(height);
			auto newSizes = GetNamespaceSizes(history);

			collectedIds.insert(removedIds.cbegin(), removedIds.cend());
			for (auto removedId : removedIds)
				m_pNamespaceById->remove(removedId);

			if (history.empty())
				m_pHistoryById->remove(history.id());

			decrementActiveSize(originalSizes.Active - newSizes.Active);
			decrementDeepSize(originalSizes.Deep - newSizes.Deep);
		});

		return collectedIds;
	}
}}
