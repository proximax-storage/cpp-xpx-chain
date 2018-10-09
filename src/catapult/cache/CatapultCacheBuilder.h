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
#include "CatapultCache.h"
#include "SubCachePluginAdapter.h"
#include "catapult/exceptions.h"

namespace catapult { namespace cache {

	/// Builder for creating a catapult cache around subcaches.
	class CatapultCacheBuilder {
	public:
		/// Adds current \a pSubCache to the builder with the specified storage traits.
		template<typename TStorageTraits, typename TCache>
		void addCurrentSubCache(std::unique_ptr<TCache>&& pSubCache) {
			add<TStorageTraits, TCache>(std::move(pSubCache), m_currentSubCaches);
		}

		/// Adds previous \a pSubCache to the builder with the specified storage traits.
		template<typename TStorageTraits, typename TCache>
		void addPreviousSubCache(std::unique_ptr<TCache>&& pSubCache) {
			add<TStorageTraits, TCache>(std::move(pSubCache), m_previousSubCaches);
		}

		/// Builds a catapult cache.
		CatapultCache buildCurrentCache() {
			CATAPULT_LOG(debug) << "creating current CatapultCache with " << m_currentSubCaches.size() << " subcaches";
			return CatapultCache(std::move(m_currentSubCaches));
		}

		/// Builds a catapult cache.
		CatapultCache buildPreviousCache() {
			CATAPULT_LOG(debug) << "creating current CatapultCache with " << m_previousSubCaches.size() << " subcaches";
			return CatapultCache(std::move(m_previousSubCaches));
		}

	private:
		std::vector<std::unique_ptr<SubCachePlugin>> m_currentSubCaches;
		std::vector<std::unique_ptr<SubCachePlugin>> m_previousSubCaches;

		template<typename TStorageTraits, typename TCache>
		void add(std::unique_ptr<TCache>&& pSubCache, std::vector<std::unique_ptr<SubCachePlugin>>& subCaches) {
			auto id = static_cast<size_t>(TCache::Id);
			subCaches.resize(std::max(subCaches.size(), id + 1));
			if (subCaches[id])
				CATAPULT_THROW_INVALID_ARGUMENT_1("subcache has already been registered with id", id);

			subCaches[id] = std::make_unique<cache::SubCachePluginAdapter<TCache, TStorageTraits>>(std::move(pSubCache));
		}
	};
}}
