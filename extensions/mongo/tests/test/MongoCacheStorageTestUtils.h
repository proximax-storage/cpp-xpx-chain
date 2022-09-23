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
#include "MongoTestUtils.h"
#include "mongo/src/MongoStorageContext.h"
#include "mongo/src/ExternalCacheStorage.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace test {

	/// Base class that provides shared utils for mongo cache storage test suites.
	template<typename TTraits>
	class MongoCacheStorageTestUtils {
		friend TTraits;
	private:
		using CacheType = typename TTraits::CacheType;
		using ElementType = typename TTraits::ModelType;

	protected:
		class CacheStorageWrapper : public PrepareDatabaseMixin {
		public:
			CacheStorageWrapper()
					: m_pMongoContext(CreateDefaultMongoStorageContext(DatabaseName()))
					, m_pCacheStorage(TTraits::CreateCacheStorage(*m_pMongoContext, CreateConfigHolder(TTraits::Network_Id)))
			{}
			template<typename TConfigHolderGenerator>
			CacheStorageWrapper(TConfigHolderGenerator ConfigHolderGenerator)
				: m_pMongoContext(CreateDefaultMongoStorageContext(DatabaseName()))
				, m_pCacheStorage(TTraits::CreateCacheStorage(*m_pMongoContext, ConfigHolderGenerator()))
			{}

		public:
			mongo::ExternalCacheStorage& get() {
				return *m_pCacheStorage;
			}

		private:
			auto CreateConfigHolder(model::NetworkIdentifier networkIdentifier) {
				test::MutableBlockchainConfiguration config;
				config.Immutable.NetworkIdentifier = networkIdentifier;
				return config::CreateMockConfigurationHolder(config.ToConst());
			}

		private:
			std::unique_ptr<mongo::MongoStorageContext> m_pMongoContext;
			std::unique_ptr<mongo::ExternalCacheStorage> m_pCacheStorage;
		};

	protected:
		static const auto& GetDelta(const cache::CatapultCacheDelta& delta) {
			return delta.template sub<CacheType>();
		}

		static size_t GetCollectionSize() {
			auto connection = CreateDbConnection();
			auto collection = connection[DatabaseName()][TTraits::Collection_Name];
			return static_cast<size_t>(collection.count_documents({}));
		}

		template<template<class, class> typename TVectorType, typename TElement, typename TAllocator, typename TFilter>
		static void AssertDbContents(const std::string collection, const TVectorType<TElement, TAllocator>& elements, TFilter filter, size_t numHiddenElements = 0) {
			auto connection = CreateDbConnection();
			auto database = connection[DatabaseName()];

			AssertCollectionSize(collection, elements.size() + numHiddenElements);
			for (const auto& element : elements)
				AssertDbContains(collection, database, element, filter);
		}

		static void AssertDbContents(const std::vector<ElementType>& elements, size_t numHiddenElements = 0) {
			AssertDbContents(TTraits::Collection_Name, elements, TTraits::GetFindFilter, numHiddenElements);
		}



	private:

		template<typename TElementType, typename TFilter>
		static void AssertDbContains(const std::string collection, mongocxx::database& database, const TElementType& element, TFilter filter) {
			auto filterResult = filter(element);
			auto matchedDocument = database[collection].find_one(filterResult.view());
			ASSERT_TRUE(matchedDocument.has_value());

			TTraits::AssertEqual(element, matchedDocument.value().view());
		}

		static void AssertDbContains(mongocxx::database& database, const ElementType& element) {
			AssertDbContains(TTraits::Collection_Name, database, element, TTraits::GetFindFilter);
		}
	};
}}
