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

#pragma once
#include "StateHashInfo.h"
#include "SubCachePlugin.h"
#include <memory>

namespace catapult { namespace cache { class ReadOnlyCatapultCache; } }

namespace catapult { namespace cache {

	/// Delta on top of a catapult cache.
	class CatapultCacheDelta {
	public:
		/// Creates a locked catapult cache delta from \a subViews.
		explicit CatapultCacheDelta(std::vector<std::unique_ptr<SubCacheView>>&& subViews);

		/// Destroys the delta.
		~CatapultCacheDelta();

	public:
		// make this class move only (the definitions are in the source file in order to allow forward declarations)
		CatapultCacheDelta(CatapultCacheDelta&&);
		CatapultCacheDelta& operator=(CatapultCacheDelta&&);

	public:
		/// Gets a specific subcache delta view.
		template<typename TCache>
		const typename TCache::CacheDeltaType& sub() const {
			return *static_cast<const typename TCache::CacheDeltaType*>(m_subViews[TCache::Id]->get());
		}

		/// Gets a specific subcache delta view.
		template<typename TCache>
		typename TCache::CacheDeltaType& sub() {
			return *static_cast<typename TCache::CacheDeltaType*>(m_subViews[TCache::Id]->get());
		}

	public:
		/// Calculates the cache state hash given \a height.
		StateHashInfo calculateStateHash(Height height) const;

		/// Sets the merkle roots for all subcaches (\a subCacheMerkleRoots).
		void setSubCacheMerkleRoots(const std::vector<Hash256>& subCacheMerkleRoots);

	public:
		/// Creates a read-only view of this delta.
		ReadOnlyCatapultCache toReadOnly() const;

	private:
		std::vector<std::unique_ptr<SubCacheView>> m_subViews;
	};
}}
