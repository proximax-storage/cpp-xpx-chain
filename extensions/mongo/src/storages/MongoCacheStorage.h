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
#include "mongo/src/MongoBulkWriter.h"
#include "mongo/src/MongoStorageContext.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/thread/FutureUtils.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include <set>
#include <tuple>
#include <unordered_set>
#include "catapult/utils/Functional.h"

namespace catapult { namespace mongo { namespace storages {

	namespace detail {

		/// Defines a mongo element filter.
		template<typename TCacheTraits, typename TElementContainerType>
		struct MongoElementFilter {
		public:
			/// Filters all elements common to added elements (\a addedElements) and removed elements (\a removedElements).
			static void RemoveCommonElements(TElementContainerType& addedElements, TElementContainerType& removedElements) {
				auto commonIds = GetCommonIds(addedElements, removedElements);
				RemoveElements(addedElements, commonIds);
				RemoveElements(removedElements, commonIds);
			}

		private:
			static auto GetCommonIds(const TElementContainerType& addedElements, const TElementContainerType& removedElements) {
				std::set<typename TCacheTraits::KeyType> addedElementIds;
				for (const auto* pAddedElement : addedElements)
					addedElementIds.insert(TCacheTraits::GetId(*pAddedElement));

				std::set<typename TCacheTraits::KeyType> removedElementIds;
				for (const auto* pRemovedElement : removedElements)
					removedElementIds.insert(TCacheTraits::GetId(*pRemovedElement));

				std::set<typename TCacheTraits::KeyType> commonIds;
				std::set_intersection(
						addedElementIds.cbegin(),
						addedElementIds.cend(),
						removedElementIds.cbegin(),
						removedElementIds.cend(),
						std::inserter(commonIds, commonIds.cbegin()));
				return commonIds;
			}

			static void RemoveElements(TElementContainerType& elements, std::set<typename TCacheTraits::KeyType>& ids) {
				for (auto iter = elements.cbegin(); elements.cend() != iter;) {
					if (ids.cend() != ids.find(TCacheTraits::GetId(**iter)))
						iter = elements.erase(iter);
					else
						++iter;
				}
			}
		};

		template<typename TCacheTraits, typename TElementContainerType>
		struct MongoMultiSetElementFilter {
			using IdContainerType = typename utils::ExpandPackTo<std::set, typename TCacheTraits::KeyTypes>::type;
		private:
			static auto GetCommonIds(const TElementContainerType& addedElements, const TElementContainerType& removedElements) {
				IdContainerType addedElementIds;
				IdContainerType removedElementIds;
				IdContainerType commonIds;
				constexpr auto size = std::tuple_size<IdContainerType>::value;
				utils::for_sequence(std::make_index_sequence<size>{}, [&](auto i){
				  for(const auto& elem : std::get<i>(addedElements))
				  {
					  std::get<i>(addedElementIds).insert(TCacheTraits::template GetId<i, std::tuple_element_t<i, typename TCacheTraits::CacheType::CacheValueTypes>, std::tuple_element_t<i, typename TCacheTraits::KeyTypes>>(*elem));
				  }
				  for(const auto& elem : std::get<i>(removedElements))
				  {
					  std::get<i>(removedElementIds).insert(TCacheTraits::template GetId<i, std::tuple_element_t<i, typename TCacheTraits::CacheType::CacheValueTypes>, std::tuple_element_t<i, typename TCacheTraits::KeyTypes>>(*elem));
				  }
				  std::set_intersection(
						  std::get<i>(addedElementIds).cbegin(),
						  std::get<i>(addedElementIds).cend(),
						  std::get<i>(removedElementIds).cbegin(),
						  std::get<i>(removedElementIds).cend(),
						  std::inserter(std::get<i>(commonIds), std::get<i>(commonIds).cbegin()));
				});
				return commonIds;
			}

			static void RemoveElements(TElementContainerType& elements, IdContainerType& ids) {

				constexpr auto size = std::tuple_size<IdContainerType>::value;
				utils::for_sequence(std::make_index_sequence<size>{}, [&](auto i){
				  auto &set = std::get<i>(elements);
				  auto &idSet = std::get<i>(ids);
					for (auto iter = set.cbegin(); set.cend() != iter;) {
						if (idSet.cend() != idSet.find(TCacheTraits::template GetId<i, std::tuple_element_t<i, typename TCacheTraits::CacheType::CacheValueTypes>, std::tuple_element_t<i, typename TCacheTraits::KeyTypes>>(**iter)))
							iter = set.erase(iter);
						else
							++iter;
					}
				});
			}
		public:
			/// Filters all elements common to added elements (\a addedElements) and removed elements (\a removedElements).
			static void RemoveCommonElements(TElementContainerType& addedElements, TElementContainerType& removedElements) {
				auto commonIds = GetCommonIds(addedElements, removedElements);
				RemoveElements(addedElements, commonIds);
				RemoveElements(removedElements, commonIds);
			}
		};
	}


	/// Defines types for mongo cache storage given a cache descriptor.
	template<typename TDescriptor>
	struct BasicMongoCacheStorageTraits {
		/// Cache type.
		using CacheType = typename TDescriptor::CacheType;

		/// Cache delta type.
		using CacheDeltaType = typename TDescriptor::CacheDeltaType;

		/// Key type.
		using KeyType = typename TDescriptor::KeyType;

		/// Model type.
		using ModelType = typename TDescriptor::ValueType;

		/// Gets the key corresponding to a value.
		static constexpr auto GetId = TDescriptor::GetKeyFromValue;
	};

	struct StorageCallbackContext {
		StorageCallbackContext(MongoErrorPolicy& policy, MongoBulkWriter& writer, Height height, const config::BlockchainConfigurationHolder& holder)
				: ErrorPolicy(policy),
				BulkWriter(writer),
				CurrentHeight(height),
				ConfigHolder(holder) {}

		MongoErrorPolicy& ErrorPolicy;
		MongoBulkWriter& BulkWriter;
		const Height CurrentHeight;
		const config::BlockchainConfigurationHolder& ConfigHolder;
	};

	namespace detail {
		namespace CallbackTag {
			struct None {};
			struct Supported {};
		};


		/// Callback static method is needed because contexpr still evaluates even if condition is never true
#define SUPPORTS_CALLBACK_DEF(CALLBACK_NAME) \
		template<typename TCacheTraits, typename T = void, typename = void> \
		struct SupportsCallback##CALLBACK_NAME { \
			typedef CallbackTag::None Type;           \
			template<typename TElements>              \
			static void Callback(StorageCallbackContext& context,  const TElements& elements, const config::BlockchainConfigurationHolder& configHolder) {\
				CATAPULT_THROW_RUNTIME_ERROR("Invalid execution!"); \
			}                                       \
		};                                          \
		template <typename TCacheTraits, typename T> \
		struct SupportsCallback##CALLBACK_NAME<TCacheTraits, T, typename std::enable_if<!std::is_member_pointer<decltype(&TCacheTraits::template CALLBACK_NAME<T>)>::value>::type> \
		{ \
			typedef CallbackTag::Supported Type;      \
			template<typename TElements>              \
			static void Callback(StorageCallbackContext& context,  const TElements& elements)\
			{\
				TCacheTraits::CALLBACK_NAME(context, elements);\
			} \
		}; \
		template <typename TCacheTraits> \
		struct SupportsCallback##CALLBACK_NAME<TCacheTraits, void, typename std::enable_if<!std::is_member_pointer<decltype(&TCacheTraits::template CALLBACK_NAME)>::value>::type> \
		{ \
			typedef CallbackTag::Supported Type;      \
            template<typename TElements>     \
			static void Callback(StorageCallbackContext& context,  const TElements& elements)\
			{\
				TCacheTraits::CALLBACK_NAME(context, elements);\
			} \
		};

		SUPPORTS_CALLBACK_DEF(RemoveCallback)
		SUPPORTS_CALLBACK_DEF(InsertCallback)

#define SUPPORTS_CALLBACK(CALLBACK_NAME) SupportsCallback##CALLBACK_NAME

		template<typename TCache, template<typename...> class TStorageType, typename ...TArgs>
		struct UnpackStorageTerms;

		template<typename TCache, template<typename...> class TStorageType, typename ...TArgs>
		struct UnpackStorageTerms<TCache, TStorageType, std::tuple<TArgs...>>
		{
			using type = TStorageType<TCache, TArgs...>;
		};
	}

	template<typename TCacheTraits>
	class MongoMultisetCacheStorage : public detail::UnpackStorageTerms<typename TCacheTraits::CacheType, ExternalMultiSetCacheStorageT, typename TCacheTraits::CacheType::CacheValueTypes>::type {
	private:
		using CacheChangesType = typename detail::UnpackStorageTerms<typename TCacheTraits::CacheDeltaType, cache::MultiSetCacheChangesT, typename TCacheTraits::CacheType::CacheValueTypes>::type;
		using ElementContainerType = typename TCacheTraits::ElementContainerType;
		using IdContainerType = typename TCacheTraits::IdContainerType;
		static constexpr int TypesSize = std::tuple_size<typename TCacheTraits::CacheType::CacheValueTypes>::value;
	public:
		/// Creates a cache storage around \a storageContext and \a networkIdentifier.
		MongoMultisetCacheStorage(MongoStorageContext& storageContext, const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
				: m_database(storageContext.createDatabaseConnection())
				, m_errorPolicy(storageContext.createCollectionErrorPolicy(std::string(TCacheTraits::Collection_Names[0])))
				, m_bulkWriter(storageContext.bulkWriter())
				, m_pConfigHolder(pConfigHolder)
		{}

	private:
		void saveDelta(const CacheChangesType& changes) override {
			auto addedElements = changes.addedElements();
 			auto modifiedElements = changes.modifiedElements();
			auto removedElements = changes.removedElements();

			// 1. remove elements common to both added and removed
			detail::MongoMultiSetElementFilter<TCacheTraits, ElementContainerType>::RemoveCommonElements(addedElements, removedElements);

			// 2. remove all removed elements from db
			removeAll(removedElements);

			// 3. insert new elements and modified elements into db
			utils::for_sequence(std::make_index_sequence<TypesSize>{}, [&](auto i){
			  std::get<i>(modifiedElements).insert(std::get<i>(addedElements).cbegin(), std::get<i>(addedElements).cend());
			});
			upsertAll(modifiedElements, changes.height());
		}
	private:
		template<int TIndex, typename TModel>
		static bsoncxx::document::value CreateFilter(const TModel* pModel) {
			return CreateFilterByKey<TIndex, std::tuple_element_t<TIndex, typename TCacheTraits::KeyTypes>>(TCacheTraits::template GetId<TIndex, TModel, std::tuple_element_t<TIndex, typename TCacheTraits::KeyTypes>>(*pModel));
		}
		template<int TIndex, typename TKey>
		static bsoncxx::document::value CreateFilterByKey(const TKey& key) {
			using namespace bsoncxx::builder::stream;

			return document() << std::string(TCacheTraits::Id_Property_Names[TIndex]) << TCacheTraits::template MapToMongoId<TIndex, TKey, std::tuple_element_t<TIndex, typename TCacheTraits::MongoKeyTypes>>(key)
							  << finalize;
		}
	private:


		void removeAll(const ElementContainerType& elements) {
			utils::for_sequence(std::make_index_sequence<TypesSize>{}, [&](auto i){
				auto &elementsSet = std::get<i>(elements);
				typedef std::remove_const_t<std::remove_reference_t<decltype(elementsSet)>> ElementBaseType;
				if (!elementsSet.empty())
				{
					if constexpr(std::is_same_v<typename detail::SUPPORTS_CALLBACK(RemoveCallback)<TCacheTraits, ElementBaseType>::Type, detail::CallbackTag::Supported>) {
						StorageCallbackContext context(m_errorPolicy, m_bulkWriter, Height(), *m_pConfigHolder);
						detail::SUPPORTS_CALLBACK(RemoveCallback)<TCacheTraits, ElementBaseType>::Callback(context, elements);
					}
					auto deleteResults = m_bulkWriter.bulkDelete(std::string(TCacheTraits::Collection_Names[i]).c_str(), elementsSet, CreateFilter<i, std::tuple_element_t<i, typename TCacheTraits::CacheType::CacheValueTypes>>).get();
					auto aggregateResult = BulkWriteResult::Aggregate(thread::get_all(std::move(deleteResults)));
					m_errorPolicy.checkDeleted(elementsSet.size(), aggregateResult, "removed elements");
				}
			});
		}
\
		void upsertAll(const ElementContainerType& elements, const Height& height) {

			auto networkIdentifier = m_pConfigHolder->Config(height).Immutable.NetworkIdentifier;
			utils::for_sequence(std::make_index_sequence<TypesSize>{}, [&](auto i){
			  auto& currentSet = std::get<i>(elements);
			  typedef std::remove_const_t<std::remove_reference_t<decltype(currentSet)>> ElementBaseType;
			  if (!currentSet.empty())
			  {
				  if constexpr(std::is_same_v<typename detail::SUPPORTS_CALLBACK(InsertCallback)<TCacheTraits, ElementBaseType>::Type, detail::CallbackTag::Supported>) {
					  StorageCallbackContext context(m_errorPolicy, m_bulkWriter, height, *m_pConfigHolder);
					  detail::SUPPORTS_CALLBACK(InsertCallback)<TCacheTraits, ElementBaseType>::Callback(context, elements);
				  }
				  auto createDocument = [&i, networkIdentifier = m_pConfigHolder->Config(height).Immutable.NetworkIdentifier](const auto* pModel, auto) {
					return TCacheTraits::template MapToMongoDocument<i, std::tuple_element_t<i, typename TCacheTraits::CacheType::CacheValueTypes>>(*pModel, networkIdentifier);
				  };
				  auto upsertResults = m_bulkWriter.bulkUpsert(std::string(TCacheTraits::Collection_Names[i]).c_str(), currentSet, createDocument, CreateFilter<i, std::tuple_element_t<i, typename TCacheTraits::CacheType::CacheValueTypes>>).get();
				  auto aggregateResult = BulkWriteResult::Aggregate(thread::get_all(std::move(upsertResults)));
				  m_errorPolicy.checkUpserted(currentSet.size(), aggregateResult, "modified and added elements");
			  }
			});
		}

	private:
		static IdContainerType GetIds(const ElementContainerType& elements) {
			IdContainerType ids;
			utils::for_sequence(std::make_index_sequence<TypesSize>{}, [&](auto i){
			  for (const auto* pElement : std::get<i>(elements))
				  std::get<i>(ids).insert(TCacheTraits::template GetId<i, std::tuple_element_t<i, typename TCacheTraits::CacheType::CacheValueTypes>, std::tuple_element_t<i, typename TCacheTraits::KeyTypes>>(*pElement));
			});
			return ids;
		}

	private:
		MongoDatabase m_database;
		MongoErrorPolicy m_errorPolicy;
		MongoBulkWriter& m_bulkWriter;
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
	};
	/// A mongo cache storage that persists historical cache data using delete and insert.
	template<typename TCacheTraits>
	class MongoHistoricalCacheStorage : public ExternalCacheStorageT<typename TCacheTraits::CacheType> {
	private:
		using CacheChangesType = cache::SingleCacheChangesT<
			typename TCacheTraits::CacheDeltaType,
			typename TCacheTraits::CacheType::CacheValueType>;
		using ElementContainerType = typename TCacheTraits::ElementContainerType;
		using IdContainerType = typename TCacheTraits::IdContainerType;

	public:
		/// Creates a cache storage around \a storageContext and \a networkIdentifier.
		MongoHistoricalCacheStorage(MongoStorageContext& storageContext, const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
				: m_database(storageContext.createDatabaseConnection())
				, m_errorPolicy(storageContext.createCollectionErrorPolicy(TCacheTraits::Collection_Name))
				, m_bulkWriter(storageContext.bulkWriter())
				, m_pConfigHolder(pConfigHolder)
		{}

	private:
		void saveDelta(const CacheChangesType& changes) override {
			auto addedElements = changes.addedElements();
			auto modifiedElements = changes.modifiedElements();
			auto removedElements = changes.removedElements();

			// 1. remove elements common to both added and removed
			detail::MongoElementFilter<TCacheTraits, ElementContainerType>::RemoveCommonElements(addedElements, removedElements);

			// 2. remove all modified and removed elements from db
			auto modifiedIds = GetIds(modifiedElements);
			auto removedIds = GetIds(removedElements);

			modifiedIds.insert(removedIds.cbegin(), removedIds.cend());
			removeAll(modifiedIds);

			// 3. insert new elements and modified elements into db
			modifiedElements.insert(addedElements.cbegin(), addedElements.cend());
			insertAll(modifiedElements, changes.height());
			if constexpr(std::is_same_v<typename detail::SUPPORTS_CALLBACK(RemoveCallback)<TCacheTraits>::Type, detail::CallbackTag::Supported>) {
				StorageCallbackContext context(m_errorPolicy, m_bulkWriter, changes.height(), *m_pConfigHolder);
				detail::SUPPORTS_CALLBACK(RemoveCallback)<TCacheTraits>::Callback(context, removedElements);
			}
			if constexpr(std::is_same_v<typename detail::SUPPORTS_CALLBACK(InsertCallback)<TCacheTraits>::Type, detail::CallbackTag::Supported>) {
				StorageCallbackContext context(m_errorPolicy, m_bulkWriter, changes.height(), *m_pConfigHolder);
				detail::SUPPORTS_CALLBACK(InsertCallback)<TCacheTraits>::Callback(context, modifiedElements);
			}
		}

	private:

		void removeAll(const IdContainerType& ids) {
			if (ids.empty())
				return;

			auto collection = m_database[TCacheTraits::Collection_Name];

			auto filter = CreateDeleteFilter(ids);
			auto deleteResult = collection.delete_many(filter.view());
			m_errorPolicy.checkDeletedAtLeast(ids.size(), BulkWriteResult(deleteResult.value().result()), "removed and modified elements");
		}

		void insertAll(const ElementContainerType& elements, const Height& height) {
			if (elements.empty())
				return;

			std::vector<typename TCacheTraits::ModelType> allModels;
			for (const auto* pElement : elements) {
				auto models = TCacheTraits::MapToMongoModels(*pElement, m_pConfigHolder->Config(height).Immutable.NetworkIdentifier);
				std::move(models.begin(), models.end(), std::back_inserter(allModels));
			}

			auto insertResults = m_bulkWriter.bulkInsert(TCacheTraits::Collection_Name, allModels, [](const auto& model, auto) {
				return TCacheTraits::MapToMongoDocument(model);
			}).get();

			auto aggregateResult = BulkWriteResult::Aggregate(thread::get_all(std::move(insertResults)));
			m_errorPolicy.checkInserted(allModels.size(), aggregateResult, "modified and added elements");
		}

	private:
		static IdContainerType GetIds(const ElementContainerType& elements) {
			IdContainerType ids;
			for (const auto* pElement : elements)
				ids.insert(TCacheTraits::GetId(*pElement));

			return ids;
		}

		static bsoncxx::document::value CreateDeleteFilter(const IdContainerType& ids) {
			using namespace bsoncxx::builder::stream;

			if (ids.empty())
				return document() << finalize;

			document doc;
			auto array = doc
					<< std::string(TCacheTraits::Id_Property_Name)
					<< open_document
						<< "$in"
						<< open_array;

			for (auto id : ids)
				array << TCacheTraits::MapToMongoId(id);

			array << close_array;
			doc << close_document;
			return doc << finalize;
		}

	private:
		MongoDatabase m_database;
		MongoErrorPolicy m_errorPolicy;
		MongoBulkWriter& m_bulkWriter;
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
	};

	/// A mongo cache storage that persists flat cache data using delete and upsert.
	template<typename TCacheTraits>
	class MongoFlatCacheStorage : public ExternalCacheStorageT<typename TCacheTraits::CacheType> {
	private:
		using CacheChangesType = cache::SingleCacheChangesT<typename TCacheTraits::CacheDeltaType, typename TCacheTraits::ModelType>;
		using KeyType = typename TCacheTraits::KeyType;
		using ModelType = typename TCacheTraits::ModelType;
		using ElementContainerType = std::unordered_set<const ModelType*>;

	public:
		/// Creates a cache storage around \a storageContext and \a networkIdentifier.
		MongoFlatCacheStorage(MongoStorageContext& storageContext, const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
				: m_database(storageContext.createDatabaseConnection())
				, m_errorPolicy(storageContext.createCollectionErrorPolicy(TCacheTraits::Collection_Name))
				, m_bulkWriter(storageContext.bulkWriter())
				, m_pConfigHolder(pConfigHolder)
		{}

	private:
		void saveDelta(const CacheChangesType& changes) override {
			auto addedElements = changes.addedElements();
			auto modifiedElements = changes.modifiedElements();
			auto removedElements = changes.removedElements();

			// 1. remove elements common to both added and removed
			detail::MongoElementFilter<TCacheTraits, ElementContainerType>::RemoveCommonElements(addedElements, removedElements);

			// 2. remove all removed elements from db
			removeAll(removedElements);

			// 3. upsert new elements and modified elements into db
			modifiedElements.insert(addedElements.cbegin(), addedElements.cend());
			upsertAll(modifiedElements, changes.height());
		}

	private:


		void removeAll(const ElementContainerType& elements) {
			if (elements.empty())
				return;

			auto deleteResults = m_bulkWriter.bulkDelete(TCacheTraits::Collection_Name, elements, CreateFilter).get();
			if constexpr(std::is_same_v<typename detail::SUPPORTS_CALLBACK(RemoveCallback)<TCacheTraits>::Type, detail::CallbackTag::Supported>) {
				StorageCallbackContext context(m_errorPolicy, m_bulkWriter, Height(), *m_pConfigHolder);
				detail::SUPPORTS_CALLBACK(RemoveCallback)<TCacheTraits>::Callback(context, elements);
			}


			auto aggregateResult = BulkWriteResult::Aggregate(thread::get_all(std::move(deleteResults)));
			m_errorPolicy.checkDeleted(elements.size(), aggregateResult, "removed elements");
		}

		void upsertAll(const ElementContainerType& elements, const Height& height) {
			if (elements.empty())
				return;

			auto createDocument = [networkIdentifier = m_pConfigHolder->Config(height).Immutable.NetworkIdentifier](const auto* pModel, auto) {
				return TCacheTraits::MapToMongoDocument(*pModel, networkIdentifier);
			};
			auto upsertResults = m_bulkWriter.bulkUpsert(TCacheTraits::Collection_Name, elements, createDocument, CreateFilter).get();
			if constexpr(std::is_same_v<typename detail::SUPPORTS_CALLBACK(InsertCallback)<TCacheTraits>::Type, detail::CallbackTag::Supported>) {
				StorageCallbackContext context(m_errorPolicy, m_bulkWriter, height, *m_pConfigHolder);
				detail::SUPPORTS_CALLBACK(InsertCallback)<TCacheTraits>::Callback(context, elements);
			}
			auto aggregateResult = BulkWriteResult::Aggregate(thread::get_all(std::move(upsertResults)));
			m_errorPolicy.checkUpserted(elements.size(), aggregateResult, "modified and added elements");
		}

	private:
		static bsoncxx::document::value CreateFilter(const ModelType* pModel) {
			return CreateFilterByKey(TCacheTraits::GetId(*pModel));
		}

		static bsoncxx::document::value CreateFilterByKey(const KeyType& key) {
			using namespace bsoncxx::builder::stream;

			return document() << std::string(TCacheTraits::Id_Property_Name) << TCacheTraits::MapToMongoId(key) << finalize;
		}

	private:
		MongoDatabase m_database;
		MongoErrorPolicy m_errorPolicy;
		MongoBulkWriter& m_bulkWriter;
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
	};
#undef SUPPORTS_CALLBACK
#undef SUPPORTS_CALLBACK_DEF

}}}
