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

#include <boost/core/span.hpp>
#include "NemesisBlockLoader.h"
#include "LocalNodeStateRef.h"
#include "NemesisFundingObserver.h"
#include "catapult/chain/BlockExecutor.h"
#include "catapult/crypto/Signer.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/observers/NotificationObserverAdapter.h"
#include "catapult/plugins/PluginManager.h"
#include "catapult/utils/IntegerMath.h"
#include "plugins/txes/config/src/model/NetworkConfigTransaction.h"
#include "plugins/txes/state_modify/src/model/ModifyStateTransaction.h"
#include "catapult/cache/CacheConstants.h"
#include "plugins/txes/config/src/cache/NetworkConfigCache.h"

namespace catapult { namespace extensions {

	namespace {
		class NemesisNotificationObserver : public observers::AggregateNotificationObserver {
		private:
			using NotificationObserverPointer1 = observers::NotificationObserverPointerT<model::BalanceTransferNotification<1>>;
			using NotificationObserverPointer2 = observers::NotificationObserverPointerT<model::Notification>;

		public:
			explicit NemesisNotificationObserver(
				NotificationObserverPointer1&& pObserver1,
				NotificationObserverPointer2&& pObserver2)
					: m_pObserver1(std::move(pObserver1))
					, m_pObserver2(std::move(pObserver2))
					, m_names{ m_pObserver1->name(), m_pObserver2->name() }
					, m_name(utils::ReduceNames(m_names))
			{}

		public:
			const std::string& name() const override {
				return m_name;
			}

			std::vector<std::string> names() const override {
				return m_names;
			}

			void notify(const model::Notification& notification, observers::ObserverContext& context) const override {
				if (model::Core_Balance_Transfer_v1_Notification == notification.Type)
					m_pObserver1->notify(static_cast<const model::BalanceTransferNotification<1>&>(notification), context);
				m_pObserver2->notify(notification, context);
			}

		private:
			NotificationObserverPointer1 m_pObserver1;
			NotificationObserverPointer2 m_pObserver2;
			std::vector<std::string> m_names;
			std::string m_name;
		};

		std::unique_ptr<const observers::NotificationObserver> PrependFundingObserver(
				const Key& nemesisPublicKey,
				NemesisFundingState& nemesisFundingState,
				std::unique_ptr<const observers::NotificationObserver>&& pObserver) {
			return std::make_unique<NemesisNotificationObserver>(CreateNemesisFundingObserver(nemesisPublicKey, nemesisFundingState), std::move(pObserver));
		}

		void LogNemesisBlockInfo(const model::BlockElement& blockElement) {
			auto networkId = blockElement.Block.Network();
			const auto& publicKey = blockElement.Block.Signer;
			const auto& generationHash = blockElement.GenerationHash;
			CATAPULT_LOG(info)
					<< std::endl
					<< "      nemesis network id: " << networkId << std::endl
					<< "      nemesis public key: " << publicKey << std::endl
					<< " nemesis generation hash: " << generationHash;
		}

		void OutputNemesisBalance(std::ostream& out, MosaicId mosaicId, Amount amount, char special = ' ') {
			out << std::endl << "      " << special << ' ' << utils::HexFormat(mosaicId) << ": " << amount;
		}

		void LogNemesisBalances(MosaicId currencyMosaicId, MosaicId harvestingMosaicId, const state::AccountBalances& balances) {
			std::ostringstream out;
			out << "Nemesis Mosaics:";
			OutputNemesisBalance(out, currencyMosaicId, balances.get(currencyMosaicId), 'C');
			OutputNemesisBalance(out, harvestingMosaicId, balances.get(harvestingMosaicId), 'H');

			for (const auto& pair : balances) {
				if (currencyMosaicId != pair.first && harvestingMosaicId != pair.first)
					OutputNemesisBalance(out, pair.first, pair.second);
			}

			CATAPULT_LOG(info) << out.str();
		}

		void CheckNemesisBlockInfo(const model::BlockElement& blockElement, const model::NetworkInfo& expectedNetwork,
				model::NetworkIdentifier expectedNetworkId, const GenerationHash& expectedGenerationHash) {
			auto networkId = blockElement.Block.Network();
			const auto& publicKey = blockElement.Block.Signer;
			const auto& generationHash = blockElement.GenerationHash;

			if (expectedNetworkId != networkId)
				CATAPULT_THROW_INVALID_ARGUMENT_1("nemesis network id does not match network", networkId);

			if (expectedNetwork.PublicKey != publicKey)
				CATAPULT_THROW_INVALID_ARGUMENT_1("nemesis public key does not match network", publicKey);

			if (expectedGenerationHash != generationHash)
				CATAPULT_THROW_INVALID_ARGUMENT_1("nemesis generation hash does not match network", generationHash);
		}

		void CheckNemesisBlockTransactionTypes(const model::Block& block, const model::TransactionRegistry& transactionRegistry) {
			for (const auto& transaction : block.Transactions()) {
				const auto* pPlugin = transactionRegistry.findPlugin(transaction.Type);
				if (!pPlugin || !pPlugin->supportsTopLevel())
					CATAPULT_THROW_INVALID_ARGUMENT_1("nemesis block contains unsupported transaction type", transaction.Type);
			}
		}

		void CheckNemesisBlockFeeMultiplier(const model::Block& block) {
			if (BlockFeeMultiplier() != block.FeeMultiplier)
				CATAPULT_THROW_INVALID_ARGUMENT_1("nemesis block contains non-zero fee multiplier", block.FeeMultiplier);
		}

		void CheckImportanceAndBalanceConsistency(Importance totalChainImportance, Amount totalChainBalance) {
			if (totalChainImportance.unwrap() != totalChainBalance.unwrap()) {
				std::ostringstream out;
				out
						<< "harvesting outflows (" << totalChainBalance << ") do not add up to power ten multiple of "
						<< "expected importance (" << totalChainImportance << ")";
				CATAPULT_THROW_INVALID_ARGUMENT(out.str().c_str());
			}
		}

		void CheckInitialCurrencyAtomicUnits(Amount expectedInitialCurrencyAtomicUnits, Amount initialCurrencyAtomicUnits) {
			if (expectedInitialCurrencyAtomicUnits != initialCurrencyAtomicUnits) {
				std::ostringstream out;
				out
						<< "currency outflows (" << initialCurrencyAtomicUnits << ") do not equal the "
						<< "expected initial currency atomic units (" << expectedInitialCurrencyAtomicUnits << ")";
				CATAPULT_THROW_INVALID_ARGUMENT(out.str().c_str());
			}
		}

		void CheckMaxMosaicAtomicUnits(const state::AccountBalances& totalFundedMosaics, Amount maxMosaicAtomicUnits) {
			for (const auto& pair : totalFundedMosaics) {
				if (maxMosaicAtomicUnits < pair.second) {
					std::ostringstream out;
					out
							<< "currency outflows (" << pair.second << ") for mosaic id " << pair.first
							<< " exceed max allowed atomic units (" << maxMosaicAtomicUnits << ")";
					CATAPULT_THROW_INVALID_ARGUMENT(out.str().c_str());
				}
			}
		}

		const model::NetworkConfigTransaction& GetNetworkConfigUpgradeTransaction(const std::shared_ptr<const model::BlockElement> pNemesisBlock) {
			for (const auto& transaction : pNemesisBlock->Transactions) {
				if (transaction.Transaction.Type == model::NetworkConfigTransaction::Entity_Type)
					return static_cast<const model::NetworkConfigTransaction&>(transaction.Transaction);
			}

			CATAPULT_THROW_RUNTIME_ERROR("network config upgrade transaction is not found in nemesis block")
		}

		const std::tuple<const model::NetworkConfiguration, const config::SupportedEntityVersions> GetStateModifyNetworkConfig(const std::shared_ptr<const model::BlockElement> pNemesisBlock) {
			std::shared_ptr<state::NetworkConfigEntry> lastEntry = nullptr;
			auto currentHeight = pNemesisBlock->Block.Height;

			for (const auto& transaction : pNemesisBlock->Transactions) {
				if (transaction.Transaction.Type == model::ModifyStateTransaction::Entity_Type)
				{
					auto tx = reinterpret_cast<const model::ModifyStateTransaction*>(&transaction.Transaction);
					auto cacheName = std::string(reinterpret_cast<const char*>(tx->CacheNamePtr()), tx->CacheNameSize);
					if(cache::GetCacheIdFromName(cacheName) == cache::CacheId::NetworkConfig)
					{
						auto span = boost::span<const uint8_t>(tx->ContentPtr(), tx->ContentSize);
						auto stateObject = cache::NetworkConfigCacheDescriptor::Serializer::DeserializeValue(span);
						if(stateObject.height() <= currentHeight && (lastEntry == nullptr || stateObject.height() > lastEntry->height())) {
							lastEntry = std::make_shared<state::NetworkConfigEntry>(stateObject);
						}
					}
				}
			}
			if(lastEntry == nullptr) {
				CATAPULT_THROW_RUNTIME_ERROR("network config upgrade transaction is not found in nemesis block")
			}
			std::istringstream inputBlock(lastEntry->networkConfig());
			std::istringstream inputVersions(lastEntry->supportedEntityVersions());
			return std::make_tuple(model::NetworkConfiguration::LoadFromBag(utils::ConfigurationBag::FromStream(inputBlock)), config::LoadSupportedEntityVersions(inputVersions));

		}
	}

	NemesisBlockLoader::NemesisBlockLoader(
			cache::CatapultCacheDelta& cacheDelta,
			const plugins::PluginManager& pluginManager,
			std::unique_ptr<const observers::NotificationObserver>&& pObserver)
			: m_cacheDelta(cacheDelta)
			, m_pluginManager(pluginManager)
			, m_pObserver(std::make_unique<observers::NotificationObserverAdapter>(
					PrependFundingObserver(m_nemesisPublicKey, m_nemesisFundingState, std::move(pObserver)),
					pluginManager.createNotificationPublisher()))
			, m_pPublisher(pluginManager.createNotificationPublisher())
	{}

	void NemesisBlockLoader::execute(const LocalNodeStateRef& stateRef, StateHashVerification stateHashVerification) {
		// 1. load the nemesis block
		auto storageView = stateRef.Storage.view();
		auto pNemesisBlockElement = storageView.loadBlockElement(stateRef.ConfigHolder->Config().Immutable.NemesisHeight);

		// 2. execute the nemesis block
		execute(stateRef.ConfigHolder, *pNemesisBlockElement, stateRef.State, stateHashVerification, Verbosity::On);
	}

	const std::tuple<const model::NetworkConfiguration, const config::SupportedEntityVersions> NemesisBlockLoader::ReadNetworkConfiguration(const std::shared_ptr<const model::BlockElement> nemesisBlock) {
		if(nemesisBlock->Block.Height != Height(1)) {
			return GetStateModifyNetworkConfig(nemesisBlock);
		}
		else {
			const auto& transaction = GetNetworkConfigUpgradeTransaction(nemesisBlock);
			auto nConfigString = std::string(reinterpret_cast<const char*>(transaction.BlockChainConfigPtr()), transaction.BlockChainConfigSize);
			auto nSupportedVersionsString = std::string(reinterpret_cast<const char*>(transaction.SupportedEntityVersionsPtr()), transaction.SupportedEntityVersionsSize);
			std::istringstream inputBlock(nConfigString);
			std::istringstream inputVersions(nSupportedVersionsString);
			return std::make_tuple(model::NetworkConfiguration::LoadFromBag(utils::ConfigurationBag::FromStream(inputBlock)), config::LoadSupportedEntityVersions(inputVersions));
		}
	}

	const std::tuple<const std::string, const std::string> NemesisBlockLoader::ReadNetworkConfigurationAsStrings(const std::shared_ptr<const model::BlockElement> nemesisBlock) {
		const auto& transaction = GetNetworkConfigUpgradeTransaction(nemesisBlock);
		auto nConfigString = std::string(reinterpret_cast<const char*>(transaction.BlockChainConfigPtr()), transaction.BlockChainConfigSize);
		auto nSupportedVersionsString = std::string(reinterpret_cast<const char*>(transaction.SupportedEntityVersionsPtr()), transaction.SupportedEntityVersionsSize);
		return std::make_tuple(nConfigString, nSupportedVersionsString);
	}

	void NemesisBlockLoader::executeAndCommit(const LocalNodeStateRef& stateRef, StateHashVerification stateHashVerification) {
		// 1. execute the nemesis block
		execute(stateRef, stateHashVerification);

		// 2. commit changes
		stateRef.Cache.commit(stateRef.ConfigHolder->Config().Immutable.NemesisHeight);
	}

	void NemesisBlockLoader::execute(const std::shared_ptr<config::BlockchainConfigurationHolder>& configHolder, const model::BlockElement& nemesisBlockElement) {
		auto catapultState = state::CatapultState();
		execute(configHolder, nemesisBlockElement, catapultState, StateHashVerification::Enabled, Verbosity::Off);
	}

	namespace {
		void RequireHashMatch(const char* hashDescription, const Hash256& blockHash, const Hash256& calculatedHash) {
			if (blockHash == calculatedHash)
				return;

			std::ostringstream out;
			out
					<< "nemesis block " << hashDescription << " hash (" << blockHash << ") does not match "
					<< "calculated " << hashDescription << " hash (" << calculatedHash << ")";
			CATAPULT_THROW_RUNTIME_ERROR(out.str().c_str());
		}

		void RequireValidSignature(const model::Block& block) {
			auto headerSize = model::VerifiableEntity::Header_Size;
			auto blockData = RawBuffer{ reinterpret_cast<const uint8_t*>(&block) + headerSize, block.GetHeaderSize() - headerSize };
			if (crypto::Verify(block.Signer, blockData, block.Signature))
				return;

			CATAPULT_THROW_RUNTIME_ERROR("nemesis block has invalid signature");
		}
	}

	void NemesisBlockLoader::execute(
			const std::shared_ptr<config::BlockchainConfigurationHolder>& configHolder,
			const model::BlockElement& nemesisBlockElement,
			state::CatapultState& catapultState,
			StateHashVerification stateHashVerification,
			Verbosity verbosity) {
		// 1. check the nemesis block
		if (Verbosity::On == verbosity)
			LogNemesisBlockInfo(nemesisBlockElement);

		const auto& config = configHolder->Config(nemesisBlockElement.Block.Height);

		CheckNemesisBlockInfo(nemesisBlockElement, config.Network.Info, config.Immutable.NetworkIdentifier, config.Immutable.GenerationHash);
		CheckNemesisBlockTransactionTypes(nemesisBlockElement.Block, m_pluginManager.transactionRegistry());
		CheckNemesisBlockFeeMultiplier(nemesisBlockElement.Block);

		// 2. reset nemesis funding observer data
		m_nemesisPublicKey = nemesisBlockElement.Block.Signer;
		m_nemesisFundingState = NemesisFundingState();

		// 3. execute the block
		auto readOnlyCache = m_cacheDelta.toReadOnly();
		auto resolverContext = m_pluginManager.createResolverContext(readOnlyCache);
		auto blockStatementBuilder = model::BlockStatementBuilder();
		auto observerState = config.Immutable.ShouldEnableVerifiableReceipts
				? observers::ObserverState(m_cacheDelta, catapultState, blockStatementBuilder)
				: observers::ObserverState(m_cacheDelta, catapultState);
		chain::ExecuteBlock(nemesisBlockElement, { *m_pObserver, resolverContext, configHolder, observerState });

		// 4. check the funded balances are reasonable
		if (Verbosity::On == verbosity)
			LogNemesisBalances(config.Immutable.CurrencyMosaicId, config.Immutable.HarvestingMosaicId, m_nemesisFundingState.TotalFundedMosaics);

		// Ignore for recreated networks
		if(config.Immutable.NemesisHeight == Height(1))
		{
			CheckImportanceAndBalanceConsistency(
					config.Network.TotalChainImportance,
					m_nemesisFundingState.TotalFundedMosaics.get(config.Immutable.HarvestingMosaicId));

			CheckInitialCurrencyAtomicUnits(
					config.Immutable.InitialCurrencyAtomicUnits,
					m_nemesisFundingState.TotalFundedMosaics.get(config.Immutable.CurrencyMosaicId));

			CheckMaxMosaicAtomicUnits(m_nemesisFundingState.TotalFundedMosaics, config.Network.MaxMosaicAtomicUnits);
		}

		// 5. check the hashes
		auto blockReceiptsHash = model::CalculateMerkleHash(*blockStatementBuilder.build());
		RequireHashMatch("receipts", nemesisBlockElement.Block.BlockReceiptsHash, blockReceiptsHash);

		// important: do not remove calculateStateHash call because it has important side-effect of populating the patricia tree delta
		auto cacheStateHash = m_cacheDelta.calculateStateHash(config.Immutable.NemesisHeight).StateHash;
		if (StateHashVerification::Enabled == stateHashVerification)
			RequireHashMatch("state", nemesisBlockElement.Block.StateHash, cacheStateHash);

		// 6. check the block signature
		RequireValidSignature(nemesisBlockElement.Block);
	}
}}
