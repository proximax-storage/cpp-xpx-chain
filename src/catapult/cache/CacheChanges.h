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
#include "CatapultCacheDelta.h"
#include <unordered_set>
#include "catapult/utils/Functional.h"
#include <vector>

namespace catapult { namespace cache {


	/// Deserialized cache changes for a single cache with multiple sets.
	/// \note This is used for tagging.
	struct MemoryCacheChanges : public utils::NonCopyable {
		virtual ~MemoryCacheChanges() = default;
	};

	/// Deserialized cache changes for a single cache.
	template<typename... TValue>
	struct MemoryCacheChangesT : public MemoryCacheChanges {
		/// Added elements.
		std::tuple<std::vector<TValue>...> Added;

		/// Removed elements.
		std::tuple<std::vector<TValue>...> Removed;

		/// Copied elements.
		std::tuple<std::vector<TValue>...> Copied;

		catapult::Height Height;
	};

	// endregion

	// region MultiSetCacheChanges
/// Provides common view of single sub cache changes.
	/// \note This is used for tagging.
	struct MultiSetCacheChanges : public utils::NonCopyable {};

	/// Provides common view of single sub cache changes.
	template<typename TCacheDelta, typename... TValues>
	class MultiSetCacheChangesT : public MultiSetCacheChanges {
	private:
		using PointerContainer = std::tuple<std::unordered_set<const TValues*>...>;

	public:
		/// Creates changes around \a cacheDelta.
		explicit MultiSetCacheChangesT(const TCacheDelta& cacheDelta, const Height& height)
				: m_pCacheDelta(&cacheDelta)
				, m_pMemoryCacheChanges(nullptr)
				, m_height(height)
		{}

		/// Creates changes around \a memoryCacheChanges.
		explicit MultiSetCacheChangesT(const MemoryCacheChangesT<TValues...>& memoryCacheChanges, const Height& height)
				: m_pCacheDelta(nullptr)
				, m_pMemoryCacheChanges(&memoryCacheChanges)
				, m_height(height)
		{}

	public:
		/// Gets pointers to all added elements.
		PointerContainer addedElements() const {
			return m_pCacheDelta ? m_pCacheDelta->addedElements() : CollectAllPointers(m_pMemoryCacheChanges->Added);
		}

		/// Gets pointers to all modified elements.
		PointerContainer modifiedElements() const {
			return m_pCacheDelta ? m_pCacheDelta->modifiedElements() : CollectAllPointers(m_pMemoryCacheChanges->Copied);
		}

		/// Gets pointers to all removed elements.
		PointerContainer removedElements() const {
			return m_pCacheDelta ? m_pCacheDelta->removedElements() : CollectAllPointers(m_pMemoryCacheChanges->Removed);
		}

		/// Gets the height of the cache change.
		Height height() const {
			return m_height;
		}

	private:
		static PointerContainer CollectAllPointers(const std::tuple<std::vector<TValues>...>& values) {
			PointerContainer pointers;
			utils::for_sequence(std::make_index_sequence<sizeof...(TValues)>{}, [&](auto i){
			  for(const auto& elem : std::get<i>(values))
			  {
				  std::get<i>(pointers).insert(&elem);
			  }
			});
			return pointers;
		}

	private:
		const TCacheDelta* m_pCacheDelta;
		const MemoryCacheChangesT<TValues...>* m_pMemoryCacheChanges;
		Height m_height;
	};
	// endregion
	// region SingleCacheChanges

	/// Provides common view of single sub cache changes.
	/// \note This is used for tagging.
	struct SingleCacheChanges : public utils::NonCopyable {};

	/// Provides common view of single sub cache changes.
	template<typename TCacheDelta, typename TValue>
	class SingleCacheChangesT : public SingleCacheChanges {
	private:
		using PointerContainer = std::unordered_set<const TValue*>;

	public:
		/// Creates changes around \a cacheDelta.
		explicit SingleCacheChangesT(const TCacheDelta& cacheDelta, const Height& height)
				: m_pCacheDelta(&cacheDelta)
				, m_pMemoryCacheChanges(nullptr)
				, m_height(height)
		{}

		/// Creates changes around \a memoryCacheChanges.
		explicit SingleCacheChangesT(const MemoryCacheChangesT<TValue>& memoryCacheChanges, const Height& height)
				: m_pCacheDelta(nullptr)
				, m_pMemoryCacheChanges(&memoryCacheChanges)
				, m_height(height)
		{}

	private:
		template<typename TElementsType>
		struct ElementFilter
		{
			static PointerContainer Filter(const TElementsType& elements)
			{
				return elements;
			}
		};


		template<typename ...TElements>
		struct ElementFilter<std::tuple<TElements...>>
		{
			static PointerContainer Filter(const std::tuple<TElements...>& elements)
			{
				return std::get<0>(elements);
			}
		};
	public:
		/// Gets pointers to all added elements.
		PointerContainer addedElements() const {
			return m_pCacheDelta ? ElementFilter<decltype(m_pCacheDelta->addedElements())>::Filter(m_pCacheDelta->addedElements()) : CollectAllPointers(std::get<0>(m_pMemoryCacheChanges->Added));
		}

		/// Gets pointers to all modified elements.
		PointerContainer modifiedElements() const {
			return m_pCacheDelta ? ElementFilter<decltype(m_pCacheDelta->modifiedElements())>::Filter(m_pCacheDelta->modifiedElements()) : CollectAllPointers(std::get<0>(m_pMemoryCacheChanges->Copied));
		}

		/// Gets pointers to all removed elements.
		PointerContainer removedElements() const {
			return m_pCacheDelta ? ElementFilter<decltype(m_pCacheDelta->removedElements())>::Filter(m_pCacheDelta->removedElements()) : CollectAllPointers(std::get<0>(m_pMemoryCacheChanges->Removed));
		}

		/// Gets the height of the cache change.
		Height height() const {
			return m_height;
		}

	private:
		static PointerContainer CollectAllPointers(const std::vector<TValue>& values) {
			PointerContainer pointers;
			for (const auto& value : values)
				pointers.insert(&value);

			return pointers;
		}

	private:
		const TCacheDelta* m_pCacheDelta;
		const MemoryCacheChangesT<TValue>* m_pMemoryCacheChanges;
		Height m_height;
	};

	// endregion

	// region CacheChanges

	/// Provides common view of aggregate cache changes.
	class CacheChanges : public utils::MoveOnly {
	public:
		/// Memory cache changes container.
		using MemoryCacheChangesContainer = std::vector<std::unique_ptr<const MemoryCacheChanges>>;

	public:
		/// Creates changes around \a cacheDelta.
		explicit CacheChanges(const CatapultCacheDelta& cacheDelta) : m_pCacheDelta(&cacheDelta)
		{}

		/// Creates changes around \a memoryCacheChangesContainer.
		explicit CacheChanges(MemoryCacheChangesContainer&& memoryCacheChangesContainer)
				: m_pCacheDelta(nullptr)
				, m_memoryCacheChangesContainer(std::move(memoryCacheChangesContainer))
		{}

	public:
		/// Gets a specific sub cache changes view.
		template<typename TCache, template<typename, typename ...> class TChangesType = SingleCacheChangesT,  typename TBaseType = typename TCache::CacheValueType, typename ...TValueTypes>
		auto sub() const {
			using SubCacheChanges = TChangesType<typename TCache::CacheDeltaType, TBaseType, TValueTypes...>;
			if (m_pCacheDelta) {
				const auto& subCacheDelta = m_pCacheDelta->sub<TCache>();
				return SubCacheChanges(subCacheDelta, subCacheDelta.height());
			}

			using TypedMemoryCacheChanges = MemoryCacheChangesT<TBaseType, TValueTypes...>;
			const auto& memoryCacheChanges = *static_cast<const TypedMemoryCacheChanges*>(m_memoryCacheChangesContainer[TCache::Id].get());
			return SubCacheChanges(memoryCacheChanges, memoryCacheChanges.Height);
		}


	private:
		const CatapultCacheDelta* m_pCacheDelta;
		MemoryCacheChangesContainer m_memoryCacheChangesContainer;
	};

	// endregion
}}
