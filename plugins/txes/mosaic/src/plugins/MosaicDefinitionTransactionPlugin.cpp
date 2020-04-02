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

#include <src/catapult/config_holder/BlockchainConfigurationHolder.h>
#include "MosaicDefinitionTransactionPlugin.h"
#include "src/model/MosaicDefinitionTransaction.h"
#include "src/model/MosaicNotifications.h"
#include "src/config/MosaicConfiguration.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "catapult/model/Address.h"
#include "catapult/plugins/PluginUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {
		
		namespace {
			using PublishEmbedded = std::function<void (const ExtendedEmbeddedMosaicDefinitionTransaction&, NotificationSubscriber&, Height)>;
			using PublishBasic = std::function<void (const MosaicDefinitionTransaction&, NotificationSubscriber&, Height)>;
			
			MosaicRentalFeeConfiguration ToMosaicRentalFeeConfiguration(
				model::NetworkIdentifier networkIdentifier,
				const model::NetworkInfo &network,
				UnresolvedMosaicId currencyMosaicId,
				const config::MosaicConfiguration &config) {
				MosaicRentalFeeConfiguration rentalFeeConfig;
				rentalFeeConfig.SinkPublicKey = config.MosaicRentalFeeSinkPublicKey;
				rentalFeeConfig.CurrencyMosaicId = currencyMosaicId;
				rentalFeeConfig.Fee = config.MosaicRentalFee;
				rentalFeeConfig.NemesisPublicKey = network.PublicKey;
				
				// sink address is already resolved but needs to be passed as unresolved into notification
				auto sinkAddress = PublicKeyToAddress(rentalFeeConfig.SinkPublicKey, networkIdentifier);
				std::memcpy(rentalFeeConfig.SinkAddress.data(), sinkAddress.data(), sinkAddress.size());
				return rentalFeeConfig;
			}
			
			
			constexpr const EmbeddedMosaicDefinitionTransaction &CastToDerivedType(const EmbeddedTransaction &transaction) {
				return static_cast<const EmbeddedMosaicDefinitionTransaction &>(transaction);
			}
			
			class MosaicDefinitionEmbeddedTransaction;
			static std::unique_ptr<MosaicDefinitionEmbeddedTransaction> CreateEmbedded(PublishEmbedded publish,
					std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder) {
				return std::make_unique<MosaicDefinitionEmbeddedTransaction>(publish, pConfigHolder);
			}
			
			class MosaicDefinitionEmbeddedTransaction : public EmbeddedTransactionPlugin {
			public:
				MosaicDefinitionEmbeddedTransaction(PublishEmbedded publish,
				                                    const std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
					: m_publish(publish)
					, m_pConfigHolder(pConfigHolder){}
				
				virtual EntityType type() const override {
					return Entity_Type_Mosaic_Definition;
				}
				
				TransactionAttributes attributes(const Height &) const override {
					auto currentVersion = MosaicDefinitionTransaction::Current_Version;
					return {3, currentVersion, utils::TimeSpan()};
				}
				
				virtual uint64_t calculateRealSize(const EmbeddedTransaction& transaction) const override {
					size_t size = sizeof(EmbeddedMosaicDefinitionTransaction);
					auto& detailed = CastToDerivedType(transaction);
					size_t sizeofProperty = sizeof(MosaicProperty);
					size += (detailed.PropertiesHeader.Count * sizeofProperty);

					if (transaction.EntityVersion() >= MosaicDefinitionTransaction::Current_Version) {
						size += sizeof(MosaicLevy);
					}

					return size;
				}
				
				/// Extracts public keys of additional accounts that must approve \a transaction.
				utils::KeySet additionalRequiredCosigners(const EmbeddedTransaction&, const config::BlockchainConfiguration&) const override {
					return utils::KeySet();
				};
				
				/// Sends all notifications from \a transaction to \a sub.
				void publish(const WeakEntityInfoT<EmbeddedTransaction>& transaction, NotificationSubscriber& sub) const override{
					m_publish(static_cast<const ExtendedEmbeddedMosaicDefinitionTransaction&>(transaction.entity()), sub, transaction.associatedHeight());
				}
			
			private:
				PublishEmbedded m_publish;
				std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
			};
			
			class MosaicDefinitionTransactionPlugin : public TransactionPlugin{
			public:
				MosaicDefinitionTransactionPlugin(PublishBasic publish, PublishEmbedded emPublish,
				                                  const std::shared_ptr<config::BlockchainConfigurationHolder> &pConfigHolder)
					: m_publish(publish)
					, m_pConfigHolder(pConfigHolder)
					, m_pEmbeddedTransactionPlugin(CreateEmbedded(emPublish, pConfigHolder)){}
			
			public:
				EntityType type() const override {
					return Entity_Type_Mosaic_Definition;
				}
				
				uint64_t calculateRealSize(const Transaction &transaction) const override {
					return MosaicDefinitionTransaction::CalculateRealSize(
						static_cast<const MosaicDefinitionTransaction &>(transaction));
				}
				
				TransactionAttributes attributes(const Height &) const override {
					auto currentVersion = MosaicDefinitionTransaction::Current_Version;
					return {3, currentVersion, utils::TimeSpan()};
				}
				
				RawBuffer dataBuffer(const Transaction &transaction) const override {
					auto headerSize = VerifiableEntity::Header_Size;
					return {reinterpret_cast<const uint8_t *>(&transaction) + headerSize,
					        transaction.Size - headerSize};
				}
				
				std::vector<RawBuffer> merkleSupplementaryBuffers(const Transaction &) const override {
					return {};
				}
				
				bool supportsTopLevel() const override {
					return true;
				}
				
				bool supportsEmbedding() const override {
					return true;
				}
				
				const EmbeddedTransactionPlugin &embeddedPlugin() const override {
					return *m_pEmbeddedTransactionPlugin;
				}
				
				void
				publish(const WeakEntityInfoT<Transaction>& transaction, NotificationSubscriber &sub) const override {
					m_publish(static_cast<const MosaicDefinitionTransaction&>(transaction.entity()), sub, transaction.associatedHeight());
				}
			
			private:
				PublishBasic m_publish;
				std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
				std::unique_ptr<EmbeddedTransactionPlugin> m_pEmbeddedTransactionPlugin;
			};
		}
		
		template<typename TTransaction>
		auto CreatePublisher(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
			return [pConfigHolder](const TTransaction& transaction, NotificationSubscriber& sub, Height associatedHeight) {
				const auto &blockChainConfig = pConfigHolder->ConfigAtHeightOrLatest(associatedHeight);
				const auto &pluginConfig = blockChainConfig.Network.template GetPluginConfiguration<config::MosaicConfiguration>();
				const auto &immutableConfig = blockChainConfig.Immutable;
				auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(immutableConfig);
				auto config = ToMosaicRentalFeeConfiguration(immutableConfig.NetworkIdentifier,
				                                             blockChainConfig.Network.Info, currencyMosaicId,
				                                             pluginConfig);
				
				auto levy = MOSAIC_DEFINITION_EXTRACT_LEVY(transaction);
				
				switch (transaction.EntityVersion()) {
					case 3:
						[[fallthrough]];
					case 4:
						// 1. sink account notification
						sub.notify(AccountPublicKeyNotification<1>(config.SinkPublicKey));
						
						// 2. rental fee charge
						// a. exempt the nemesis account
						if (config.NemesisPublicKey != transaction.Signer) {
							sub.notify(BalanceTransferNotification<1>(transaction.Signer, config.SinkAddress,
							                                          config.CurrencyMosaicId, config.Fee));
							sub.notify(MosaicRentalFeeNotification<1>(transaction.Signer, config.SinkAddress,
							                                          config.CurrencyMosaicId, config.Fee));
						}
						
						// 3. registration
						sub.notify(MosaicNonceNotification<1>(transaction.Signer, transaction.MosaicNonce,
						                                      transaction.MosaicId));
						sub.notify(
							MosaicPropertiesNotification<1>(transaction.PropertiesHeader, transaction.PropertiesPtr()));
						
						sub.notify(MosaicDefinitionNotification<1>(
							transaction.Signer,
							transaction.MosaicId,
							ExtractAllProperties(transaction.PropertiesHeader, transaction.PropertiesPtr()),
							levy));
						break;
					
					default:
						CATAPULT_LOG(debug) << "invalid version of MosaicDefinitionTransaction: "
						                    << transaction.EntityVersion();
				}
			};
		}
		
		std::unique_ptr<TransactionPlugin> CreateMosaicDefinitionTransactionPlugin(
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
			return std::make_unique<MosaicDefinitionTransactionPlugin>(
				CreatePublisher<MosaicDefinitionTransaction>(pConfigHolder),
				CreatePublisher<ExtendedEmbeddedMosaicDefinitionTransaction>(pConfigHolder),
				pConfigHolder);
		}
	}}
