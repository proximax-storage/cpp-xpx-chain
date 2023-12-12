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

#include "NotificationPublisher.h"
#include "Block.h"
#include "BlockUtils.h"
#include "FeeUtils.h"
#include "Address.h"
#include "NotificationSubscriber.h"
#include "TransactionPlugin.h"

namespace catapult { namespace model {

	namespace {
		void RequireKnown(EntityType entityType) {
			if (BasicEntityType::Other == ToBasicEntityType(entityType))
				CATAPULT_THROW_RUNTIME_ERROR_1("NotificationPublisher only supports Block and Transaction entities", entityType);
		}

		class BasicNotificationPublisher : public NotificationPublisher {
		public:
			BasicNotificationPublisher(const TransactionRegistry& transactionRegistry, UnresolvedMosaicId feeMosaicId)
					: m_transactionRegistry(transactionRegistry)
					, m_feeMosaicId(feeMosaicId)
			{}

		public:
			void publish(const WeakEntityInfoT<VerifiableEntity>& entityInfo, NotificationSubscriber& sub) const override {
				RequireKnown(entityInfo.type());

				const auto& entity = entityInfo.entity();
				auto basicEntityType = ToBasicEntityType(entityInfo.type());

				// 1. publish source change notification
				publishSourceChange(basicEntityType, sub);

				// 2. publish common notifications
				sub.notify(AccountPublicKeyNotification<1>(entity.Signer));

				// 3. publish entity specific notifications
				const auto* pBlockHeader = entityInfo.isAssociatedBlockHeaderSet() ? &entityInfo.associatedBlockHeader() : nullptr;
				switch (basicEntityType) {
				case BasicEntityType::Block:
					return publish(static_cast<const Block&>(entity), sub);

				case BasicEntityType::Transaction:
					return publish(static_cast<const Transaction&>(entity), entityInfo.hash(), pBlockHeader, entityInfo.associatedHeight(), sub);

				default:
					return;
				}
			}

		private:
			void publishSourceChange(BasicEntityType basicEntityType, NotificationSubscriber& sub) const {
				using Notification = SourceChangeNotification<1>;

				switch (basicEntityType) {
				case BasicEntityType::Block:
					// set block source to zero (source ids are 1-based)
					sub.notify(Notification(Notification::SourceChangeType::Absolute, 0, Notification::SourceChangeType::Absolute, 0));
					break;

				case BasicEntityType::Transaction:
					// set transaction source (source ids are 1-based)
					sub.notify(Notification(Notification::SourceChangeType::Relative, 1, Notification::SourceChangeType::Absolute, 0));
					break;

				default:
					break;
				}
			}

			void publish(const Block& block, NotificationSubscriber& sub) const {
				// raise an entity notification
				auto headerSize = VerifiableEntity::Header_Size;
				auto blockData = RawBuffer{ reinterpret_cast<const uint8_t*>(&block) + headerSize, block.GetHeaderSize() - headerSize };
				switch (block.EntityVersion()) {
				case 4: {
					// Before balance change notificaitons can be issued, signer must be validated
					sub.notify(SignerNotification<1>(block.Signer));


					// raise an account public key notification
					if (Key() != block.Beneficiary)
						sub.notify(AccountPublicKeyNotification<1>(block.Beneficiary));

					sub.notify(EntityNotification<1>(block.Network(), block.Type, block.EntityVersion(), block.SignatureDerivationScheme()));

					// raise a block notification
					auto blockTransactionsInfo = CalculateBlockTransactionsInfo(block);
					BlockNotification<1> blockNotification(block.Signer, block.Beneficiary, block.Timestamp, block.Difficulty, block.FeeInterest, block.FeeInterestDenominator);
					blockNotification.NumTransactions = blockTransactionsInfo.Count;
					blockNotification.TotalFee = blockTransactionsInfo.TotalFee;

					sub.notify(blockNotification);

					// raise a signature notification
					switch (block.SignatureDerivationScheme())
					{
						case DerivationScheme::Unset: //unset: prior to merge
						{
							sub.notify(BlockSignatureNotification<1>(block.Signer, block.Signature, blockData));
							break;
						}
						default:
						{
							sub.notify(BlockSignatureNotification<2>(block.Signer, block.Signature, block.SignatureDerivationScheme(), blockData));
						}
					}

					// raise signer importance notification
					sub.notify(BlockSignerImportanceNotification<1>(block.Signer));
					break;
				}
				case 3: {
					// raise an account public key notification
					if (Key() != block.Beneficiary)
						sub.notify(AccountPublicKeyNotification<1>(block.Beneficiary));

					sub.notify(EntityNotification<1>(block.Network(), block.Type, block.EntityVersion(), block.SignatureDerivationScheme()));

					// raise a block notification
					auto blockTransactionsInfo = CalculateBlockTransactionsInfo(block);
					BlockNotification<1> blockNotification(block.Signer, block.Beneficiary, block.Timestamp, block.Difficulty, block.FeeInterest, block.FeeInterestDenominator);
					blockNotification.NumTransactions = blockTransactionsInfo.Count;
					blockNotification.TotalFee = blockTransactionsInfo.TotalFee;

					sub.notify(blockNotification);

					// raise a signature notification
					switch (block.SignatureDerivationScheme())
					{
						case DerivationScheme::Unset: //unset: prior to merge
						{
							sub.notify(BlockSignatureNotification<1>(block.Signer, block.Signature, blockData));
							break;
						}
						default:
						{
							sub.notify(BlockSignatureNotification<2>(block.Signer, block.Signature, block.SignatureDerivationScheme(), blockData));
						}
					}
					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of Block: " << block.EntityVersion();
				}
			}

			void publish(
					const Transaction& transaction,
					const Hash256& hash,
					const BlockHeader* pBlockHeader,
					const Height& height,
					NotificationSubscriber& sub) const {
				const auto& plugin = *m_transactionRegistry.findPlugin(transaction.Type);
				auto attributes = plugin.attributes(height);

				// raise an entity notification
				sub.notify(EntityNotification<1>(transaction.Network(), transaction.Type, transaction.EntityVersion(), transaction.SignatureDerivationScheme()));

				// raise transaction notifications
				auto fee = pBlockHeader ?
					CalculateTransactionFee(pBlockHeader->FeeMultiplier, transaction, pBlockHeader->FeeInterest, pBlockHeader->FeeInterestDenominator)
					: transaction.MaxFee;
				sub.notify(TransactionNotification<1>(transaction.Signer, hash, transaction.Type, transaction.Deadline));
				sub.notify(TransactionDeadlineNotification<1>(transaction.Deadline, attributes.MaxLifetime));

				// Before balance change notificaitons can be issued, signer must be validated
				sub.notify(SignerNotification<1>(transaction.Signer));

				sub.notify(TransactionFeeNotification<1>(transaction.Size, fee, transaction.MaxFee));
				sub.notify(BalanceDebitNotification<1>(transaction.Signer, m_feeMosaicId, fee));

				// raise a signature notification
				switch (transaction.SignatureDerivationScheme())
				{
					case DerivationScheme::Unset: //unset: prior to merge
					{

						sub.notify(SignatureNotification<1>(
								transaction.Signer,
								transaction.Signature,
								plugin.dataBuffer(transaction),
								Replay::ReplayProtectionMode::Enabled));
						break;
					}
					default:
					{
						sub.notify(SignatureNotification<2>(
								transaction.Signer,
								transaction.Signature,
								transaction.SignatureDerivationScheme(),
								plugin.dataBuffer(transaction),
								Replay::ReplayProtectionMode::Enabled));
					}
				}

			}

		private:
			const TransactionRegistry& m_transactionRegistry;
			UnresolvedMosaicId m_feeMosaicId;
		};

		class CustomNotificationPublisher : public NotificationPublisher {
		public:
			explicit CustomNotificationPublisher(const TransactionRegistry& transactionRegistry)
					: m_transactionRegistry(transactionRegistry)
			{}

		public:
			void publish(const WeakEntityInfoT<VerifiableEntity>& entityInfo, NotificationSubscriber& sub) const override {
				RequireKnown(entityInfo.type());

				if (BasicEntityType::Transaction != ToBasicEntityType(entityInfo.type()))
					return;

				return publish(static_cast<const Transaction&>(entityInfo.entity()), entityInfo.hash(), entityInfo.associatedHeight(), sub);
			}

			void publish(const Transaction& transaction, const Hash256& hash, const Height& associatedHeight, NotificationSubscriber& sub) const {
				const auto& plugin = *m_transactionRegistry.findPlugin(transaction.Type);
				PublishContext context;
				context.SignerAddress =  model::PublicKeyToAddress(transaction.Signer, transaction.Network());
				context.AssociatedHeight = associatedHeight;
				plugin.publish(WeakEntityInfoT<Transaction>(transaction, hash, associatedHeight), context,  sub);
			}

		private:
			const TransactionRegistry& m_transactionRegistry;
		};

		class AllNotificationPublisher : public NotificationPublisher {
		public:
			AllNotificationPublisher(const TransactionRegistry& transactionRegistry, UnresolvedMosaicId feeMosaicId)
					: m_basicPublisher(transactionRegistry, feeMosaicId)
					, m_customPublisher(transactionRegistry)
			{}

		public:
			void publish(const WeakEntityInfoT<VerifiableEntity>& entityInfo, NotificationSubscriber& sub) const override {
				m_basicPublisher.publish(entityInfo, sub);
				m_customPublisher.publish(entityInfo, sub);
			}

		private:
			BasicNotificationPublisher m_basicPublisher;
			CustomNotificationPublisher m_customPublisher;
		};
	}

	std::unique_ptr<NotificationPublisher> CreateNotificationPublisher(
			const TransactionRegistry& transactionRegistry,
			UnresolvedMosaicId feeMosaicId,
			PublicationMode mode) {
		switch (mode) {
		case PublicationMode::Basic:
			return std::make_unique<BasicNotificationPublisher>(transactionRegistry, feeMosaicId);

		case PublicationMode::Custom:
			return std::make_unique<CustomNotificationPublisher>(transactionRegistry);

		default:
			return std::make_unique<AllNotificationPublisher>(transactionRegistry, feeMosaicId);
		}
	}
}}
