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

#include "MongoTransactionStatusStorage.h"
#include "MongoBulkWriter.h"
#include "mappers/MapperUtils.h"
#include "mappers/TransactionStatusMapper.h"
#include "catapult/model/TransactionStatus.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo {

	namespace {
		constexpr auto Collection_Name = "transactionStatuses";

		auto CreateFilter(const std::string& fieldName) {
			return [fieldName](const auto& status) {
				auto filter = document()
						<< fieldName
						<< open_document
							<< "$eq" << mongo::mappers::ToBinary(status.Hash)
						<< close_document
						<< finalize;
				return filter;
			};
		}

		class MongoTransactionStatusStorage final : public subscribers::TransactionStatusSubscriber {
		public:
			MongoTransactionStatusStorage(MongoStorageContext& context)
					: m_context(context)
					, m_database(m_context.createDatabaseConnection())
					, m_errorPolicy(m_context.createCollectionErrorPolicy(Collection_Name))
			{}

		public:
			void notifyStatus(const model::Transaction& transaction, const Height&, const Hash256& hash, uint32_t status) override {
				std::unique_lock lock(m_mutex);
				m_transactionStatuses.emplace_back(hash, status, transaction.Deadline);
			}

			void flush() override {
				decltype(m_transactionStatuses) transactionStatuses;
				{
					std::unique_lock lock(m_mutex);
					transactionStatuses = std::move(m_transactionStatuses);
				}

				if (transactionStatuses.empty())
					return;

				// upsert into transaction statuses collection
				auto results = m_context.bulkWriter().bulkUpsert(
						Collection_Name,
						transactionStatuses,
						[](const auto& status, auto) { return mappers::ToDbModel(status); },
						CreateFilter("hash")).get();
				auto aggregateUpsert = BulkWriteResult::Aggregate(thread::get_all(std::move(results)));
				m_errorPolicy.checkUpserted(transactionStatuses.size(), aggregateUpsert, "transaction statuses");
			}

		private:
			MongoStorageContext& m_context;
			MongoDatabase m_database;
			MongoErrorPolicy m_errorPolicy;
			std::vector<model::TransactionStatus> m_transactionStatuses;
			std::shared_mutex m_mutex;
		};
	}

	std::unique_ptr<subscribers::TransactionStatusSubscriber> CreateMongoTransactionStatusStorage(MongoStorageContext& context) {
		return std::make_unique<MongoTransactionStatusStorage>(context);
	}
}}
