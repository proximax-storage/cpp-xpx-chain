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

#include <catapult/crypto/Hashes.h>
#include "AggregateTransactionPlugin.h"
#include "src/config/AggregateConfiguration.h"
#include "src/model/AggregateNotifications.h"
#include "src/model/AggregateTransaction.h"
#include "catapult/model/SupercontractNotifications.h"
#include "catapult/model/TransactionPlugin.h"
#include "AggregateCommon.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		constexpr const AggregateTransaction& CastToDerivedType(const Transaction& transaction) {
			return static_cast<const AggregateTransaction&>(transaction);
		}

		class AggregateTransactionPlugin : public TransactionPlugin {
		public:
			AggregateTransactionPlugin(
				const TransactionRegistry& transactionRegistry,
				model::EntityType transactionType,
				const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
					: m_transactionRegistry(transactionRegistry)
					, m_transactionType(transactionType)
					, m_pConfigHolder(pConfigHolder)
			{}

		public:
			EntityType type() const override {
				return m_transactionType;
			}

			TransactionAttributes attributes(const Height& height) const override {
				const auto& config = m_pConfigHolder->ConfigAtHeightOrLatest(height);
				const auto& pluginConfig = config.Network.template GetPluginConfiguration<config::AggregateConfiguration>();
				return { AggregateTransaction::Min_Version, AggregateTransaction::Current_Version, pluginConfig.MaxBondedTransactionLifetime };
			}

			uint64_t calculateRealSize(const Transaction& transaction) const override {
				// if size is valid, the real size is the transaction size
				// if size is invalid, return a size that can never be correct (transaction size is uint32_t)
				return IsSizeValid(CastToDerivedType(transaction), m_transactionRegistry)
						? transaction.Size
						: std::numeric_limits<uint64_t>::max();
			}

			void publish(const WeakEntityInfoT<Transaction>& transactionInfo, NotificationSubscriber& sub) const override {
				const auto& aggregate = CastToDerivedType(transactionInfo.entity());
				auto numTransactions = static_cast<uint32_t>(std::distance(aggregate.Transactions().cbegin(), aggregate.Transactions().cend()));
				auto numCosignatures = aggregate.CosignaturesCount();

				switch (aggregate.EntityVersion()) {
				case 4: {

					sub.notify(AggregateTransactionTypeNotification<2>(m_transactionType));

					std::set<Key> signers;
					for (const auto& subTransaction : aggregate.Transactions()) {
						signers.insert(subTransaction.Signer);
					}

					Hash256 payloadHash;
					crypto::Sha3_256_Builder hasher;
					hasher.update(utils::RawBuffer { reinterpret_cast<const uint8_t*>(&aggregate.MaxFee),
													 sizeof(aggregate.MaxFee) });
					hasher.update(utils::RawBuffer { reinterpret_cast<const uint8_t*>(aggregate.TransactionsPtr()),
													 GetTransactionPayloadSize(aggregate) });
					hasher.final(payloadHash);

					sub.notify(ReleasedTransactionsNotification<1>(signers, payloadHash));

					// publish all sub-transaction information
					for (const auto& subTransaction : aggregate.Transactions()) {
						// - change source
						constexpr auto Relative = SourceChangeNotification<1>::SourceChangeType::Relative;
						sub.notify(SourceChangeNotification<1>(Relative, 0, Relative, 1));

						const auto& plugin = m_transactionRegistry.findPlugin(subTransaction.Type)->embeddedPlugin();

						sub.notify(EntityNotification<1>(
								subTransaction.Network(),
								subTransaction.Type,
								subTransaction.EntityVersion()));

						// - specific sub-transaction notifications
						//   (calculateRealSize would have failed if plugin is unknown or not embeddable)
						WeakEntityInfoT<EmbeddedTransaction> subTransactionInfo{
							ConvertEmbeddedTransaction(subTransaction, aggregate.Deadline, sub.mempool()),
							transactionInfo.associatedHeight()
						};
						plugin.publish(subTransactionInfo, sub);
					}

					break;
				}

				case 3: {
					sub.notify(AggregateTransactionHashNotification<1>(
						transactionInfo.hash(),
						numTransactions,
						aggregate.TransactionsPtr()));

					sub.notify(AggregateCosignaturesNotification<2>(
							aggregate.Signer,
							numTransactions,
							aggregate.TransactionsPtr(),
							numCosignatures,
							aggregate.CosignaturesPtr()));

					[[fallthrough]];
				}

				case 2: {
					sub.notify(AggregateTransactionTypeNotification<1>(m_transactionType));

					// publish aggregate notifications
					// (notice that this must be raised before embedded transaction notifications in order for cosigner aggregation to work)
					sub.notify(AggregateCosignaturesNotification<1>(
							aggregate.Signer,
							numTransactions,
							aggregate.TransactionsPtr(),
							numCosignatures,
							aggregate.CosignaturesPtr()));

					// publish all sub-transaction information
					for (const auto& subTransaction : aggregate.Transactions()) {
						// - change source
						constexpr auto Relative = SourceChangeNotification<1>::SourceChangeType::Relative;
						sub.notify(SourceChangeNotification<1>(Relative, 0, Relative, 1));

						// - signers and entity
						sub.notify(AccountPublicKeyNotification<1>(subTransaction.Signer));
						const auto& plugin = m_transactionRegistry.findPlugin(subTransaction.Type)->embeddedPlugin();

						sub.notify(EntityNotification<1>(
								subTransaction.Network(),
								subTransaction.Type,
								subTransaction.EntityVersion()));

						// - generic sub-transaction notification
						sub.notify(AggregateEmbeddedTransactionNotification<1>(
								aggregate.Signer,
								subTransaction,
								numCosignatures,
								aggregate.CosignaturesPtr()));

						// - specific sub-transaction notifications
						//   (calculateRealSize would have failed if plugin is unknown or not embeddable)
						WeakEntityInfoT<EmbeddedTransaction> subTransactionInfo{
							ConvertEmbeddedTransaction(subTransaction, aggregate.Deadline, sub.mempool()),
							transactionInfo.associatedHeight()
						};
						plugin.publish(subTransactionInfo, sub);
					}

					// publish all cosigner information (as an optimization these are published with the source of the last sub-transaction)
					const auto* pCosignature = aggregate.CosignaturesPtr();
					for (auto i = 0u; i < numCosignatures; ++i) {
						// - notice that all valid cosigners must have been observed previously as part of either
						//   (1) sub-transaction execution or (2) composite account setup
						// - require the cosigners to sign the aggregate indirectly via the hash of its data
						sub.notify(SignatureNotification<1>(pCosignature->Signer, pCosignature->Signature, transactionInfo.hash()));
						++pCosignature;
					}
					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of AggregateTransaction: " << aggregate.EntityVersion();
				}
			}

			RawBuffer dataBuffer(const Transaction& transaction) const override {
				const auto& aggregate = CastToDerivedType(transaction);

				auto headerSize = VerifiableEntity::Header_Size;
				return {
					reinterpret_cast<const uint8_t*>(&aggregate) + headerSize,
					sizeof(AggregateTransaction) - headerSize + aggregate.PayloadSize
				};
			}

			std::vector<RawBuffer> merkleSupplementaryBuffers(const Transaction& transaction) const override {
				const auto& aggregate = CastToDerivedType(transaction);

				std::vector<RawBuffer> buffers;
				auto numCosignatures = aggregate.CosignaturesCount();
				const auto* pCosignature = aggregate.CosignaturesPtr();
				for (auto i = 0u; i < numCosignatures; ++i) {
					buffers.push_back(pCosignature->Signer);
					++pCosignature;
				}

				return buffers;
			}

			bool supportsTopLevel() const override {
				return true;
			}

			bool supportsEmbedding() const override {
				return false;
			}

			const EmbeddedTransactionPlugin& embeddedPlugin() const override {
				CATAPULT_THROW_RUNTIME_ERROR("aggregate transaction is not embeddable");
			}

		private:
			const TransactionRegistry& m_transactionRegistry;
			model::EntityType m_transactionType;
			std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
		};
	}

	std::unique_ptr<TransactionPlugin> CreateAggregateTransactionPlugin(
			const TransactionRegistry& transactionRegistry,
			model::EntityType transactionType,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return std::make_unique<AggregateTransactionPlugin>(transactionRegistry, transactionType, pConfigHolder);
	}
}}
