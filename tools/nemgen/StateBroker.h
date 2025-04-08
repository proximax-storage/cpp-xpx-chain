/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/TransactionPlugin.h"
#include "AccountMigrationManager.h"
#include "catapult/state/AccountState.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "Converters.h"
#include "catapult/cache/ExchangeCache.h"
#include "catapult/cache/MosaicCache.h"
#include "catapult/cache/CatapultCache.h"
#include "catapult/cache/MetadataV1Cache.h"
#include "catapult/cache/MetadataCache.h"
#include "catapult/cache/LevyCache.h"
#include "catapult/cache/MultisigCache.h"
#include "catapult/cache/PropertyCache.h"
#include "catapult/cache/BcDriveCache.h"
#include "catapult/cache/NamespaceCache.h"
#include "catapult/builders/StateModifyBuilder.h"
#include "catapult/cache/NetworkConfigCache.h"
#include "catapult/cache/CommitteeCache.h"

namespace catapult { namespace tools {  namespace nemgen {

	template<typename T>
	struct CacheKeyStringConverter {
		static auto ConvertKey(const T& key) {
			return std::string(key.begin(), key.end());
		}
	};

	template <typename T, typename Y>
	struct CacheKeyStringConverter<utils::BaseValue<T, Y>> {
		static auto ConvertKey(const utils::BaseValue<T, Y>& key) {
			return std::string(reinterpret_cast<const char*>(&key), sizeof(T));
		}
	};

	template<typename TCache, typename TDescriptor>
	static auto BuildStateTransaction(const model::NetworkIdentifier identifier, const Key& nemesisPublicKey, cache::CacheId cacheId, cache::SubCacheId subCacheId, const typename TDescriptor::ValueType& record) {
		auto transactionBuilder = builders::ModifyStateBuilder(identifier, nemesisPublicKey);
		transactionBuilder.setKey(CacheKeyStringConverter<std::decay_t<decltype(TDescriptor::GetKeyFromValue(record))>>::ConvertKey(TDescriptor::GetKeyFromValue(record)));
		transactionBuilder.setCacheId(cacheId, subCacheId);
		std::string value = TDescriptor::Serializer::SerializeValue(record);
		auto buffer = utils::RawBuffer(reinterpret_cast<uint8_t*>(value.data()), value.size());
		transactionBuilder.setData(buffer);
		return transactionBuilder.build();
	}

	template<typename TCacheType, typename TSetTypes, typename TDescriptor = typename TCacheType::Descriptor>
	struct StateRebuilder {
		template<typename TExecutor>
		static void Rebuild(const model::NetworkIdentifier identifier, const Key& nemesisPublicKey, const cache::CatapultCacheDelta& catapultCache, utils::AccountMigrationManager& manager, Height height, cache::SubCacheId subCacheId, TExecutor executor) {
			auto& stateCache = catapultCache.sub<TCacheType>();
			auto cacheDeltaView = stateCache.template tryMakeBroadIterableView<typename TSetTypes::BaseSetDeltaType>();
			//std::vector<typename TSetTypes::BaseSetType::ElementType> results;
			for(const auto& val : *cacheDeltaView) {
				//results.push_back(Convert(manager, val.second));
				auto transaction = BuildStateTransaction<TCacheType, TDescriptor>(identifier, nemesisPublicKey, static_cast<cache::CacheId>(TCacheType::Id), subCacheId, Convert(manager, val.second));
				executor(std::move(transaction));
			}
		}
	};

	template<typename TDescriptor>
	struct StateRebuilder<cache::NetworkConfigCache, cache::NetworkConfigCacheTypes::PrimaryTypes, TDescriptor> {
		template<typename TExecutor>
		static void Rebuild(const model::NetworkIdentifier identifier, const Key& nemesisPublicKey, const cache::CatapultCacheDelta& catapultCache, utils::AccountMigrationManager& manager, Height height, cache::SubCacheId subCacheId, TExecutor executor, std::unique_ptr<model::NetworkConfiguration>& lastActiveEntry) {
			auto& stateCache = catapultCache.sub<cache::NetworkConfigCache>();
			auto cacheDeltaView = stateCache.template tryMakeBroadIterableView<typename cache::NetworkConfigCacheTypes::PrimaryTypes::BaseSetDeltaType>();

			// Pre-emptively check which is the last currently active network configuration
			auto currentlyActiveHeight = Height(1);
			for(const auto& val : *cacheDeltaView) {
				auto entryHeight = val.second.height();
				if(entryHeight < height && entryHeight > currentlyActiveHeight)
					currentlyActiveHeight = entryHeight;
			}

			for(const auto& val : *cacheDeltaView) {
				state::NetworkConfigEntry entry = Convert(manager, val.second, val.second.height() >= currentlyActiveHeight);
				if(entry.height() == currentlyActiveHeight) {
					std::istringstream inputBlock(entry.networkConfig());
					lastActiveEntry = std::make_unique<model::NetworkConfiguration>(model::NetworkConfiguration::LoadFromBag(utils::ConfigurationBag::FromStream(inputBlock)));
				}
				auto transaction = BuildStateTransaction<cache::NetworkConfigCache, TDescriptor>(identifier, nemesisPublicKey, static_cast<cache::CacheId>(cache::NetworkConfigCache::Id), subCacheId, entry);
				executor(std::move(transaction));
			}

			//TODO Possible work on the currently active entry.
		}
	};

	class StateBroker {
	public:

		StateBroker(const cache::CatapultCacheDelta& catapultCache, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder, std::string nemesisPrivateKey) :  m_catapultCache(catapultCache), m_accountMigrationManager(nemesisPrivateKey, "paging", pConfigHolder), m_pHolder(pConfigHolder), m_NemesisAccount(crypto::KeyPair::FromString(nemesisPrivateKey)){

		}

	public:
		template<typename TExecutor>
		void ProcessCaches(Height height, TExecutor executor) {
			/// Network config must be the first cache to be converted, so that the account manager properly sets the nemesis account.
			StateRebuilder<cache::NetworkConfigCache, cache::NetworkConfigCacheTypes::PrimaryTypes>::Rebuild(m_pHolder->Config().Immutable.NetworkIdentifier, m_NemesisAccount.publicKey(), m_catapultCache, m_accountMigrationManager, height, static_cast<cache::SubCacheId>(cache::GeneralSubCache::Main), executor, m_activeConfiguration);
			StateRebuilder<cache::AccountStateCache, cache::AccountStateCacheTypes::PrimaryTypes>::Rebuild(m_pHolder->Config().Immutable.NetworkIdentifier, m_NemesisAccount.publicKey(), m_catapultCache, m_accountMigrationManager, height, static_cast<cache::SubCacheId>(cache::GeneralSubCache::Main), executor);
			// Config update must not be set for nemesis block
			StateRebuilder<cache::ExchangeCache, cache::ExchangeCacheTypes::PrimaryTypes>::Rebuild(m_pHolder->Config().Immutable.NetworkIdentifier, m_NemesisAccount.publicKey(), m_catapultCache, m_accountMigrationManager, height,  static_cast<cache::SubCacheId>(cache::GeneralSubCache::Main), executor);
			StateRebuilder<cache::MosaicCache, cache::MosaicCacheTypes::PrimaryTypes>::Rebuild(m_pHolder->Config().Immutable.NetworkIdentifier, m_NemesisAccount.publicKey(), m_catapultCache, m_accountMigrationManager, height,  static_cast<cache::SubCacheId>(cache::GeneralSubCache::Main), executor);
			StateRebuilder<cache::MosaicCache, cache::MosaicCacheTypes::HeightGroupingTypes, cache::MosaicCacheTypes::HeightGroupingTypesDescriptor>::Rebuild(m_pHolder->Config().Immutable.NetworkIdentifier, m_NemesisAccount.publicKey(), m_catapultCache, m_accountMigrationManager, height,  static_cast<cache::SubCacheId>(cache::GeneralSubCache::Height), executor);
			StateRebuilder<cache::MetadataV1Cache, cache::MetadataV1CacheTypes::PrimaryTypes>::Rebuild(m_pHolder->Config().Immutable.NetworkIdentifier, m_NemesisAccount.publicKey(), m_catapultCache, m_accountMigrationManager, height,  static_cast<cache::SubCacheId>(cache::GeneralSubCache::Main), executor);
			//StateRebuilder<cache::MetadataV1Cache, cache::MetadataV1CacheTypes::HeightGroupingTypes, cache::MetadataV1CacheTypes::HeightGroupingTypesDescriptor>::Rebuild(m_pHolder->Config().Immutable.NetworkIdentifier, m_NemesisAccount->publicKey(), m_catapultCache, m_accountMigrationManager, height,  static_cast<cache::SubCacheId>(cache::GeneralSubCache::Height), executor);
			StateRebuilder<cache::MetadataCache, cache::MetadataCacheTypes::PrimaryTypes>::Rebuild(m_pHolder->Config().Immutable.NetworkIdentifier, m_NemesisAccount.publicKey(), m_catapultCache, m_accountMigrationManager, height,  static_cast<cache::SubCacheId>(cache::GeneralSubCache::Main), executor);
			StateRebuilder<cache::LevyCache, cache::LevyCacheTypes::PrimaryTypes>::Rebuild(m_pHolder->Config().Immutable.NetworkIdentifier, m_NemesisAccount.publicKey(), m_catapultCache, m_accountMigrationManager, height,  static_cast<cache::SubCacheId>(cache::GeneralSubCache::Main), executor);
			//RebuildStates<cache::LevyCache, cache::LevyCacheTypes::HeightGroupingTypes, cache::LevyCacheTypes::HeightGroupingTypesDescriptor>(m_catapultCache, m_accountMigrationManager, height, static_cast<cache::SubCacheId>(cache::GeneralSubCache::Height), executor);
			StateRebuilder<cache::MultisigCache, cache::MultisigCacheTypes::PrimaryTypes>::Rebuild(m_pHolder->Config().Immutable.NetworkIdentifier, m_NemesisAccount.publicKey(), m_catapultCache, m_accountMigrationManager, height, static_cast<cache::SubCacheId>(cache::GeneralSubCache::Main), executor);
			StateRebuilder<cache::CommitteeCache, cache::CommitteeCacheTypes::PrimaryTypes>::Rebuild(m_pHolder->Config().Immutable.NetworkIdentifier, m_NemesisAccount.publicKey(), m_catapultCache, m_accountMigrationManager, height, static_cast<cache::SubCacheId>(cache::GeneralSubCache::Main), executor);
			StateRebuilder<cache::NamespaceCache, cache::NamespaceCacheTypes::PrimaryTypes>::Rebuild(m_pHolder->Config().Immutable.NetworkIdentifier, m_NemesisAccount.publicKey(), m_catapultCache, m_accountMigrationManager, height, static_cast<cache::SubCacheId>(cache::GeneralSubCache::Main), executor);
			StateRebuilder<cache::NamespaceCache, cache::NamespaceCacheTypes::HeightGroupingTypes, cache::NamespaceCacheTypes::HeightGroupingTypesDescriptor>::Rebuild(m_pHolder->Config().Immutable.NetworkIdentifier, m_NemesisAccount.publicKey(), m_catapultCache, m_accountMigrationManager, height, static_cast<cache::SubCacheId>(cache::GeneralSubCache::Height), executor);
			StateRebuilder<cache::NamespaceCache, cache::NamespaceCacheTypes::FlatMapTypes, cache::NamespaceCacheTypes::FlatMapTypesDescriptor>::Rebuild(m_pHolder->Config().Immutable.NetworkIdentifier, m_NemesisAccount.publicKey(), m_catapultCache, m_accountMigrationManager, height, static_cast<cache::SubCacheId>(cache::GeneralSubCache::Secondary), executor);
			StateRebuilder<cache::PropertyCache, cache::PropertyCacheTypes::PrimaryTypes>::Rebuild(m_pHolder->Config().Immutable.NetworkIdentifier, m_NemesisAccount.publicKey(), m_catapultCache, m_accountMigrationManager, height, static_cast<cache::SubCacheId>(cache::GeneralSubCache::Main), executor);
		}

		inline void Dump(std::string path) {
			m_accountMigrationManager.writeToFile(path);
		}

		model::NetworkConfiguration& GetActiveConfiguration() {
			return *m_activeConfiguration;
		}
	private:
		utils::AccountMigrationManager m_accountMigrationManager;
		const cache::CatapultCacheDelta& m_catapultCache;
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pHolder;
		crypto::KeyPair m_NemesisAccount;
		std::unique_ptr<model::NetworkConfiguration> m_activeConfiguration;

	};


}}}
