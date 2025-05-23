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
#include "ExternalCacheStorageBuilder.h"
#include "MongoReceiptPlugin.h"
#include "MongoStorageContext.h"
#include "MongoTransactionPlugin.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/plugins.h"
#include "catapult/model/TransactionFeeCalculator.h"

namespace catapult { namespace mongo {

	/// A manager for registering mongo plugins.
	class MongoPluginManager {
	public:
		/// Creates a new plugin manager around \a mongoContext and \a pConfigHolder.
		explicit MongoPluginManager(MongoStorageContext& mongoContext,
									const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
									std::shared_ptr<model::TransactionFeeCalculator> pTransactionFeeCalculator)
				: m_mongoContext(mongoContext)
				, m_pConfigHolder(pConfigHolder)
				, m_pTransactionFeeCalculator(std::move(pTransactionFeeCalculator))
		{}

	public:
		/// Gets the mongo storage context.
		MongoStorageContext& mongoContext() const {
			return m_mongoContext;
		}

		/// Gets the configuration holder.
		const std::shared_ptr<config::BlockchainConfigurationHolder>& configHolder() const {
			return m_pConfigHolder;
		}

	public:
		/// Adds support for a transaction described by \a pTransactionPlugin.
		void addTransactionSupport(std::unique_ptr<MongoTransactionPlugin>&& pTransactionPlugin) {
			m_transactionRegistry.registerPlugin(std::move(pTransactionPlugin));
		}

		/// Adds support for a receipt described by \a pReceiptPlugin.
		void addReceiptSupport(std::unique_ptr<MongoReceiptPlugin>&& pReceiptPlugin) {
			m_receiptRegistry.registerPlugin(std::move(pReceiptPlugin));
		}

		/// Adds support for an external cache storage described by \a pStorage.
		template<typename TStorage>
		void addStorageSupport(std::unique_ptr<TStorage>&& pStorage) {
			m_storageBuilder.add(std::move(pStorage));
		}

	public:
		/// Gets the transaction registry.
		const MongoTransactionRegistry& transactionRegistry() const {
			return m_transactionRegistry;
		}

		/// Gets the receipt registry.
		const MongoReceiptRegistry& receiptRegistry() const {
			return m_receiptRegistry;
		}

		const model::TransactionFeeCalculator& transactionFeeCalculator() const {
			return *m_pTransactionFeeCalculator;
		}

		/// Creates an external cache storage.
		std::unique_ptr<ExternalCacheStorage> createStorage() {
			return m_storageBuilder.build();
		}

	private:
		MongoStorageContext& m_mongoContext;
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
		MongoTransactionRegistry m_transactionRegistry;
		MongoReceiptRegistry m_receiptRegistry;
		std::shared_ptr<model::TransactionFeeCalculator> m_pTransactionFeeCalculator;
		ExternalCacheStorageBuilder m_storageBuilder;
	};
}}

/// Entry point for registering a dynamic module with \a manager.
extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager);
