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
#include "Transaction.h"
#include "TransactionPlugin.h"
#include "catapult/functions.h"

namespace catapult { namespace model {

	/// Transaction plugin factory options.
	enum class TransactionPluginFactoryOptions {
		/// Transaction supports both top-level and embedding.
		Default,

		/// Transaction only supports embedding.
		Only_Embeddable
	};

	/// Factory for creating transaction plugins.
	template<TransactionPluginFactoryOptions Options>
	class TransactionPluginFactory {
	public:
		/// Creates an embedded transaction plugin around \a publishEmbeddedFunc.
		template<typename TEmbeddedTransaction, typename TExtendedEmbeddedTransaction, typename TPublishEmbeddedFunc>
		static std::unique_ptr<EmbeddedTransactionPlugin> CreateEmbedded(TPublishEmbeddedFunc publishEmbeddedFunc) {
			return std::make_unique<EmbeddedTransactionPluginT<TEmbeddedTransaction, TExtendedEmbeddedTransaction>>(publishEmbeddedFunc);
		}

		/// Creates a transaction plugin that supports embedding around \a publishFunc and \a publishEmbeddedFunc.
		template<typename TTransaction, typename TEmbeddedTransaction, typename TExtendedEmbeddedTransaction, typename TPublishFunc, typename TPublishEmbeddedFunc>
		static std::unique_ptr<TransactionPlugin> Create(TPublishFunc publishFunc, TPublishEmbeddedFunc publishEmbeddedFunc) {
			return std::make_unique<TransactionPluginT<TTransaction, TEmbeddedTransaction, TExtendedEmbeddedTransaction>>(publishFunc, publishEmbeddedFunc);
		}

	private:
		template<typename TTransaction, typename TDerivedTransaction, typename TExtendedDerivedTransaction, typename TPlugin>
		class BasicTransactionPluginT : public TPlugin {
		private:
			using PublishFunc = consumer<const TExtendedDerivedTransaction&, const Height&, NotificationSubscriber&>;

		public:
			explicit BasicTransactionPluginT(const PublishFunc& publishFunc) : m_publishFunc(publishFunc)
			{}

		public:
			EntityType type() const override {
				return TDerivedTransaction::Entity_Type;
			}

			TransactionAttributes attributes(const Height&) const override {
				auto version = TDerivedTransaction::Current_Version;
				return { version, version, utils::TimeSpan() };
			}

			uint64_t calculateRealSize(const TTransaction& transaction) const override {
				return TDerivedTransaction::CalculateRealSize(static_cast<const TDerivedTransaction&>(transaction));
			}

		protected:
			void publishImpl(const WeakEntityInfoT<TTransaction>& transactionInfo, NotificationSubscriber& sub) const {
				m_publishFunc(static_cast<const TExtendedDerivedTransaction&>(transactionInfo.entity()), transactionInfo.associatedHeight(), sub);
			}

		private:
			PublishFunc m_publishFunc;
		};

		template<typename TEmbeddedTransaction, typename TExtendedEmbeddedTransaction>
		class EmbeddedTransactionPluginT
				: public BasicTransactionPluginT<EmbeddedTransaction, TEmbeddedTransaction, TExtendedEmbeddedTransaction, EmbeddedTransactionPlugin> {
		private:
			using BaseType = BasicTransactionPluginT<EmbeddedTransaction, TEmbeddedTransaction, TExtendedEmbeddedTransaction, EmbeddedTransactionPlugin>;

		public:
			template<typename TPublishEmbeddedFunc>
			explicit EmbeddedTransactionPluginT(TPublishEmbeddedFunc publishEmbeddedFunc) : BaseType(publishEmbeddedFunc)
			{}

		public:
			utils::KeySet additionalRequiredCosigners(const EmbeddedTransaction& transaction, const config::BlockchainConfiguration& config) const override {
				if constexpr (TransactionPluginFactoryOptions::Default == Options) {
#ifdef _MSC_VER
					// suppress warning that transaction and config are unreferenced formal parameters
					(transaction);
					(config);
#endif
					return utils::KeySet();
				} else {
					return ExtractAdditionalRequiredCosigners(static_cast<const TEmbeddedTransaction&>(transaction), config);
				}
			}

			void publish(const WeakEntityInfoT<EmbeddedTransaction>& transactionInfo, NotificationSubscriber& sub) const override {
				BaseType::publishImpl(transactionInfo, sub);
			}
		};

		template<typename TTransaction, typename TEmbeddedTransaction, typename TExtendedEmbeddedTransaction>
		class TransactionPluginT : public BasicTransactionPluginT<Transaction, TTransaction, TTransaction, TransactionPlugin> {
		private:
			using BaseType = BasicTransactionPluginT<Transaction, TTransaction, TTransaction, TransactionPlugin>;

		public:
			template<typename TPublishFunc, typename TPublishEmbeddedFunc>
			explicit TransactionPluginT(TPublishFunc publishFunc, TPublishEmbeddedFunc publishEmbeddedFunc)
					: BaseType(publishFunc)
					, m_pEmbeddedTransactionPlugin(CreateEmbedded<TEmbeddedTransaction, TExtendedEmbeddedTransaction>(publishEmbeddedFunc))
			{}

		public:
			void publish(const WeakEntityInfoT<Transaction>& transactionInfo, NotificationSubscriber& sub) const override {
				BaseType::publishImpl(transactionInfo, sub);
			}

			RawBuffer dataBuffer(const Transaction& transaction) const override {
				auto headerSize = VerifiableEntity::Header_Size;
				return { reinterpret_cast<const uint8_t*>(&transaction) + headerSize, transaction.Size - headerSize };
			}

			std::vector<RawBuffer> merkleSupplementaryBuffers(const Transaction&) const override {
				return {};
			}

			bool supportsTopLevel() const override {
				return TransactionPluginFactoryOptions::Default == Options;
			}

			bool supportsEmbedding() const override {
				return true;
			}

			const EmbeddedTransactionPlugin& embeddedPlugin() const override {
				return *m_pEmbeddedTransactionPlugin;
			}

		private:
			std::unique_ptr<EmbeddedTransactionPlugin> m_pEmbeddedTransactionPlugin;
		};
	};

/// Defines a transaction plugin factory for \a NAME transaction with \a OPTIONS using \a PUBLISH.
#define DEFINE_TRANSACTION_PLUGIN_FACTORY(NAME, OPTIONS, PUBLISH) \
	std::unique_ptr<TransactionPlugin> Create##NAME##TransactionPlugin() { \
		using Factory = TransactionPluginFactory<TransactionPluginFactoryOptions::OPTIONS>; \
		return Factory::Create<NAME##Transaction, Embedded##NAME##Transaction, ExtendedEmbedded##NAME##Transaction>( \
				PUBLISH<NAME##Transaction>, \
				PUBLISH<ExtendedEmbedded##NAME##Transaction>); \
	}

/// Defines a transaction plugin factory for \a NAME transaction with \a OPTIONS using \a PUBLISH accepting \a CONFIG_TYPE configuration.
#define DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(NAME, OPTIONS, PUBLISH, CONFIG_TYPE) \
	std::unique_ptr<TransactionPlugin> Create##NAME##TransactionPlugin(const CONFIG_TYPE& config) { \
		using Factory = TransactionPluginFactory<TransactionPluginFactoryOptions::OPTIONS>; \
		return Factory::Create<NAME##Transaction, Embedded##NAME##Transaction, ExtendedEmbedded##NAME##Transaction>( \
				PUBLISH<NAME##Transaction>(config), \
				PUBLISH<ExtendedEmbedded##NAME##Transaction>(config)); \
	}
}}
