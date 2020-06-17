/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoService.h"
#include "MongoBulkWriter.h"
#include "MongoStorageContext.h"
#include "mappers/MapperUtils.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/extensions/ServiceState.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo {

	namespace {
		constexpr auto Service_Name = "Mongo";
		constexpr auto Transaction_Statuses_Collection = "transactionStatuses";

		class MongoServiceRegistrar : public extensions::ServiceRegistrar {
		public:
			MongoServiceRegistrar(const std::shared_ptr<MongoStorageContext>& pContext, const std::shared_ptr<const MongoTransactionRegistry>& pRegistry)
					: m_pContext(pContext)
					, m_pRegistry(pRegistry)
			{}

		public:
			extensions::ServiceRegistrarInfo info() const override {
				return { Service_Name, extensions::ServiceRegistrarPhase::Initial_With_Modules };
			}

			void registerServiceCounters(extensions::ServiceLocator&) override {
				// no additional counters
			}

			void registerServices(extensions::ServiceLocator& locator, extensions::ServiceState& state) override {
				locator.registerRootedService(Service_Name, std::make_shared<MongoServices>(m_pContext, m_pRegistry));

				state.hooks().addTransactionsChangeHandler([pContext = m_pContext](const auto& changeInfo) {
					auto createFilter = [](const Hash256* pHash) {
						return document() << "hash" << mappers::ToBinary(*pHash) << finalize;
					};

					pContext->bulkWriter().bulkDelete(Transaction_Statuses_Collection, changeInfo.AddedTransactionHashes, createFilter).get();
				});
			}

		private:
			class MongoServices {
			public:
				MongoServices(
					const std::shared_ptr<const MongoStorageContext>& pContext,
					const std::shared_ptr<const MongoTransactionRegistry>& pRegistry)
					: m_pContext(pContext)
					, m_pRegistry(pRegistry)
				{}

			private:
				std::shared_ptr<const MongoStorageContext> m_pContext;
				std::shared_ptr<const MongoTransactionRegistry> m_pRegistry;
			};

		private:
			std::shared_ptr<MongoStorageContext> m_pContext;
			std::shared_ptr<const MongoTransactionRegistry> m_pRegistry;
		};
	}

	DECLARE_SERVICE_REGISTRAR(Mongo)(const std::shared_ptr<MongoStorageContext>& pContext, const std::shared_ptr<const MongoTransactionRegistry>& pRegistry) {
		return std::make_unique<MongoServiceRegistrar>(pContext, pRegistry);
	}
}}
