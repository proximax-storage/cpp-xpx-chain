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

#include "CatapultCache.h"
#include "CacheHeight.h"
#include "ReadOnlyCatapultCache.h"
#include "SubCachePluginAdapter.h"
#include "catapult/crypto/Hashes.h"
#include "catapult/utils/StackLogger.h"

namespace catapult { namespace cache {

	namespace {
		template<typename TSubCacheViews>
		std::vector<const void*> ExtractReadOnlyViews(const TSubCacheViews& subViews) {
			std::vector<const void*> readOnlyViews;
			for (const auto& pSubView : subViews) {
				if (!pSubView) {
					readOnlyViews.push_back(nullptr);
					continue;
				}

				readOnlyViews.push_back(pSubView->asReadOnly());
			}

			return readOnlyViews;
		}

		template<typename TSubCacheViews, typename TUpdateMerkleRoot>
		std::vector<Hash256> CollectSubCacheMerkleRoots(TSubCacheViews& subViews, TUpdateMerkleRoot updateMerkleRoot) {
			std::vector<Hash256> merkleRoots;
			for (const auto& pSubView : subViews) {
				Hash256 merkleRoot;
				if (!pSubView || !pSubView->enabled())
					continue;

				updateMerkleRoot(*pSubView);
				if (pSubView->tryGetMerkleRoot(merkleRoot))
					merkleRoots.push_back(merkleRoot);
			}

			return merkleRoots;
		}

		Hash256 CalculateStateHash(const std::vector<Hash256>& subCacheMerkleRoots) {
			Hash256 stateHash;
			if (subCacheMerkleRoots.empty()) {
				stateHash = Hash256();
			} else {
				RawBuffer subCacheMerkleRootsBuffer{
					reinterpret_cast<const uint8_t*>(subCacheMerkleRoots.data()),
					subCacheMerkleRoots.size() * Hash256_Size
				};
				crypto::Sha3_256(subCacheMerkleRootsBuffer, stateHash);
			}

			return stateHash;
		}

		template<typename TSubCacheViews, typename TUpdateMerkleRoot>
		StateHashInfo CalculateStateHashInfo(const TSubCacheViews& subViews, TUpdateMerkleRoot updateMerkleRoot) {
			utils::SlowOperationLogger logger("CalculateStateHashInfo", utils::LogLevel::Warning);

			StateHashInfo stateHashInfo;
			stateHashInfo.SubCacheMerkleRoots = CollectSubCacheMerkleRoots(subViews, updateMerkleRoot);
			stateHashInfo.StateHash = CalculateStateHash(stateHashInfo.SubCacheMerkleRoots);
			return stateHashInfo;
		}

		template<typename TSubCacheViews>
		void LogSubCacheNames(const TSubCacheViews& subViews) {
			for (const auto& pSubView : subViews) {
				if (!pSubView || !pSubView->supportsMerkleRoot() || !pSubView->enabled())
					continue;

				SubCacheViewIdentifier id = pSubView->id();
				std::string cacheName(id.CacheName.begin(), id.CacheName.end());
				CATAPULT_LOG(debug) << "sub cache " << cacheName << " [" << id.CacheId << "]";
			}
		}
	}

	// region CatapultCacheView

	CatapultCacheView::CatapultCacheView(CacheHeightView&& cacheHeightView, std::vector<std::unique_ptr<const SubCacheView>>&& subViews)
			: m_pCacheHeight(std::make_unique<CacheHeightView>(std::move(cacheHeightView)))
			, m_subViews(std::move(subViews))
	{}

	CatapultCacheView::~CatapultCacheView() = default;

	CatapultCacheView::CatapultCacheView(CatapultCacheView&&) = default;

	CatapultCacheView& CatapultCacheView::operator=(CatapultCacheView&&) = default;

	StateHashInfo CatapultCacheView::calculateStateHash() const {
		return CalculateStateHashInfo(m_subViews, [](const auto&) {});
	}

	Height CatapultCacheView::height() const {
		return m_pCacheHeight->get();
	}

	ReadOnlyCatapultCache CatapultCacheView::toReadOnly() const {
		return ReadOnlyCatapultCache(ExtractReadOnlyViews(m_subViews));
	}

	// endregion

	// region CatapultCacheDelta

	CatapultCacheDelta::CatapultCacheDelta(std::vector<std::unique_ptr<SubCacheView>>&& subViews)
			: m_subViews(std::move(subViews))
	{}

	CatapultCacheDelta::~CatapultCacheDelta() = default;

	CatapultCacheDelta::CatapultCacheDelta(CatapultCacheDelta&&) = default;

	CatapultCacheDelta& CatapultCacheDelta::operator=(CatapultCacheDelta&&) = default;

	StateHashInfo CatapultCacheDelta::calculateStateHash(const Height& height) const {
		return CalculateStateHashInfo(m_subViews, [height](auto& subView) { subView.updateMerkleRoot(height); });
	}

	void CatapultCacheDelta::setSubCacheMerkleRoots(const std::vector<Hash256>& subCacheMerkleRoots) {
		auto merkleRootIndex = 0u;
		for (const auto& pSubView : m_subViews) {
			if (!pSubView || !pSubView->supportsMerkleRoot() || !pSubView->enabled())
				continue;

			if (merkleRootIndex == subCacheMerkleRoots.size()) {
				LogSubCacheNames(m_subViews);
				for (const auto& merkleRoot : subCacheMerkleRoots)
					CATAPULT_LOG(debug) << "sub cache merkle root " << merkleRoot;

				CATAPULT_THROW_INVALID_ARGUMENT_1("too few sub cache merkle roots were passed", subCacheMerkleRoots.size());
			}

			// this will always succeed because supportsMerkleRoot was checked above
			pSubView->trySetMerkleRoot(subCacheMerkleRoots[merkleRootIndex++]);
		}

		if (merkleRootIndex != subCacheMerkleRoots.size()) {
			CATAPULT_THROW_INVALID_ARGUMENT_2(
					"wrong number of sub cache merkle roots were passed (expected, actual)",
					merkleRootIndex,
					subCacheMerkleRoots.size());
		}
	}

	void CatapultCacheDelta::setHeight(const Height& height) {
		for (auto& pSubView : m_subViews) {
			if (!!pSubView)
				pSubView->setHeight(height);
		}
	}

	void CatapultCacheDelta::backupChanges(bool replace) {
		for (auto& pSubView : m_subViews) {
			if (!!pSubView && pSubView->enabled())
				pSubView->backupChanges(replace);
		}
	}

	void CatapultCacheDelta::restoreChanges() {
		for (auto& pSubView : m_subViews) {
			if (!!pSubView && pSubView->enabled())
				pSubView->restoreChanges();
		}
	}

	ReadOnlyCatapultCache CatapultCacheDelta::toReadOnly() const {
		return ReadOnlyCatapultCache(ExtractReadOnlyViews(m_subViews));
	}

	// endregion

	// region CatapultCacheDetachableDelta

	CatapultCacheDetachableDelta::CatapultCacheDetachableDelta(
			CacheHeightView&& cacheHeightView,
			std::vector<std::unique_ptr<DetachedSubCacheView>>&& detachedSubViews,
			const Height& heightDelta)
			// note that CacheHeightView is a unique_ptr to allow CatapultCacheDetachableDelta to be declared without it defined
			: m_pCacheHeightView(std::make_unique<CacheHeightView>(std::move(cacheHeightView)))
			, m_detachedDelta(std::move(detachedSubViews))
			, m_heightDelta(heightDelta)
	{}

	CatapultCacheDetachableDelta::~CatapultCacheDetachableDelta() = default;

	CatapultCacheDetachableDelta::CatapultCacheDetachableDelta(CatapultCacheDetachableDelta&&) = default;

	Height CatapultCacheDetachableDelta::height() const {
		return m_pCacheHeightView->get() + m_heightDelta;
	}

	CatapultCacheDetachedDelta CatapultCacheDetachableDelta::detach() {
		return std::move(m_detachedDelta);
	}

	// endregion

	// region CatapultCacheDetachedDelta

	CatapultCacheDetachedDelta::CatapultCacheDetachedDelta(std::vector<std::unique_ptr<DetachedSubCacheView>>&& detachedSubViews)
			: m_detachedSubViews(std::move(detachedSubViews))
	{}

	CatapultCacheDetachedDelta::~CatapultCacheDetachedDelta() = default;

	CatapultCacheDetachedDelta::CatapultCacheDetachedDelta(CatapultCacheDetachedDelta&&) = default;

	CatapultCacheDetachedDelta& CatapultCacheDetachedDelta::operator=(CatapultCacheDetachedDelta&&) = default;

	std::unique_ptr<CatapultCacheDelta> CatapultCacheDetachedDelta::tryLock() {
		std::vector<std::unique_ptr<SubCacheView>> subViews;
		for (const auto& pDetachedSubView : m_detachedSubViews) {
			if (!pDetachedSubView) {
				subViews.push_back(nullptr);
				continue;
			}

			auto pSubView = pDetachedSubView->tryLock();
			if (!pSubView)
				return nullptr;

			subViews.push_back(std::move(pSubView));
		}

		return std::make_unique<CatapultCacheDelta>(std::move(subViews));
	}

	// endregion

	// region CatapultCache

	namespace {
		template<typename TResultView, typename TSubCaches, typename TMapper>
		std::vector<std::unique_ptr<TResultView>> MapSubCaches(TSubCaches& subCaches, TMapper map, bool includeNulls = true) {
			std::vector<std::unique_ptr<TResultView>> resultViews;
			for (const auto& pSubCache : subCaches) {
				auto pSubCacheView = pSubCache ? map(pSubCache) : nullptr;
				if (!pSubCacheView && !includeNulls)
					continue;

				resultViews.push_back(std::move(pSubCacheView));
			}

			return resultViews;
		}
	}

	CatapultCache::CatapultCache(std::vector<std::unique_ptr<SubCachePlugin>>&& subCaches)
			: m_pConfigHeight(std::make_unique<CacheHeight>())
			, m_pCacheHeight(std::make_unique<CacheHeight>())
			, m_subCaches(std::move(subCaches))
	{}

	CatapultCache::~CatapultCache() = default;

	CatapultCache::CatapultCache(CatapultCache&&) = default;

	CatapultCache& CatapultCache::operator=(CatapultCache&&) = default;

	CatapultCacheView CatapultCache::createView() const {
		// acquire a height reader lock to ensure the view is composed of consistent subcache views
		auto pCacheHeightView = m_pCacheHeight->view();
		auto subViews = MapSubCaches<const SubCacheView>(m_subCaches, [&pCacheHeightView](const auto& pSubCache) { return pSubCache->createView(pCacheHeightView.get()); });
		return CatapultCacheView(std::move(pCacheHeightView), std::move(subViews));
	}

	CatapultCacheDelta CatapultCache::createDelta() {
		// since only one subcache delta is allowed outstanding at a time and an outstanding delta is required for commit,
		// subcache deltas will always be consistent
		auto pCacheHeightView = m_pCacheHeight->view();
		auto subViews = MapSubCaches<SubCacheView>(m_subCaches, [&pCacheHeightView](const auto& pSubCache) { return pSubCache->createDelta(pCacheHeightView.get()); });
		return CatapultCacheDelta(std::move(subViews));
	}

	CatapultCacheDetachableDelta CatapultCache::createDetachableDelta(const Height& heightDelta) const {
		// acquire a height reader lock to ensure the delta is composed of consistent subcache deltas
		auto pCacheHeightView = m_pCacheHeight->view();
		auto detachedSubViews = MapSubCaches<DetachedSubCacheView>(m_subCaches, [&pCacheHeightView, heightDelta](const auto& pSubCache) {
			return pSubCache->createDetachedDelta(pCacheHeightView.get() + heightDelta);
		});
		return CatapultCacheDetachableDelta(std::move(pCacheHeightView), std::move(detachedSubViews), heightDelta);
	}

	void CatapultCache::commit(Height height) {
		// use the height writer lock to lock the entire cache during commit
		auto cacheHeightModifier = m_pCacheHeight->modifier();
		// Updates height of config before commit
		m_pConfigHeight->modifier().set(height);

		for (const auto& pSubCache : m_subCaches) {
			if (pSubCache)
				pSubCache->commit();
		}

		// finally, update the cache height
		cacheHeightModifier.set(height);
	}

	Height CatapultCache::height() const {
		return m_pCacheHeight->view().get();
	}

	Height CatapultCache::configHeight() const {
		return m_pConfigHeight->view().get();
	}

	std::vector<std::unique_ptr<const CacheStorage>> CatapultCache::storages() const {
		return MapSubCaches<const CacheStorage>(
				m_subCaches,
				[](const auto& pSubCache) { return pSubCache->createStorage(); },
				false);
	}

	std::vector<std::unique_ptr<CacheStorage>> CatapultCache::storages() {
		return MapSubCaches<CacheStorage>(
				m_subCaches,
				[](const auto& pSubCache) { return pSubCache->createStorage(); },
				false);
	}

	std::vector<std::unique_ptr<const CacheChangesStorage>> CatapultCache::changesStorages() const {
		return MapSubCaches<const CacheChangesStorage>(
				m_subCaches,
				[](const auto& pSubCache) { return pSubCache->createChangesStorage(); },
				false);
	}

	void CatapultCache::addSubCache(std::unique_ptr<SubCachePlugin> pSubCache) {
		if (!pSubCache)
			return;

		auto id = pSubCache->id();
		m_subCaches.resize(std::max(m_subCaches.size(), id + 1));
		if (m_subCaches[id])
			CATAPULT_THROW_INVALID_ARGUMENT_1("subcache has already been registered with id", id);

		m_subCaches[id] = std::move(pSubCache);
	}

	// endregion
}}
