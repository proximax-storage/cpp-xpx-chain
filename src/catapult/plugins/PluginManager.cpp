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

#include "PluginManager.h"

namespace catapult { namespace plugins {

	PluginManager::PluginManager(
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			const StorageConfiguration& storageConfig)
			: m_pConfigHolder(pConfigHolder)
			, m_storageConfig(storageConfig)
			, m_shouldEnableVerifiableState(immutableConfig().ShouldEnableVerifiableState)
			, m_pTransactionFeeCalculator(std::make_shared<model::TransactionFeeCalculator>())
	{}

	// region config

	const model::NetworkConfiguration& PluginManager::config(const Height& height) const {
		return m_pConfigHolder->Config(height).Network;
	}

	const model::NetworkConfiguration& PluginManager::config() const {
		return m_pConfigHolder->Config().Network;
	}

	const std::shared_ptr<config::BlockchainConfigurationHolder>& PluginManager::configHolder() const {
		return m_pConfigHolder;
	}

	const StorageConfiguration& PluginManager::storageConfig() const {
		return m_storageConfig;
	}

	const config::ImmutableConfiguration& PluginManager::immutableConfig() const {
		return m_pConfigHolder->Config().Immutable;
	}

	cache::CacheConfiguration PluginManager::cacheConfig(const std::string& name) const {
		if (!m_storageConfig.PreferCacheDatabase)
			return cache::CacheConfiguration();

		return cache::CacheConfiguration(
				(boost::filesystem::path(m_storageConfig.CacheDatabaseDirectory) / name).generic_string(),
				m_storageConfig.MaxCacheDatabaseWriteBatchSize,
				m_shouldEnableVerifiableState ? cache::PatriciaTreeStorageMode::Enabled : cache::PatriciaTreeStorageMode::Disabled);
	}

	void PluginManager::setShouldEnableVerifiableState(bool shouldEnableVerifiableState) {
		m_shouldEnableVerifiableState = shouldEnableVerifiableState;
	}

	// endregion

	// region transactions

	void PluginManager::addTransactionSupport(std::unique_ptr<model::TransactionPlugin>&& pTransactionPlugin) {
		m_transactionRegistry.registerPlugin(std::move(pTransactionPlugin));
	}

	const model::TransactionRegistry& PluginManager::transactionRegistry() const {
		return m_transactionRegistry;
	}

	// endregion

	// region cache

	void PluginManager::addCacheSupport(std::unique_ptr<cache::SubCachePlugin>&& pSubCachePlugin) {
		m_cacheBuilder.add(std::move(pSubCachePlugin));
	}

	cache::CatapultCache PluginManager::createCache() {
		return m_cacheBuilder.build();
	}

	void PluginManager::updateCache(cache::CatapultCache& cache) {
		m_cacheBuilder.update(cache);
	}

	// endregion

	namespace {
		template<typename TBuilder, typename THooks, typename... TArgs>
		static void ApplyAll(TBuilder& builder, const THooks& hooks, TArgs&&... args) {
			for (const auto& hook : hooks)
				hook(builder, std::forward<TArgs>(args)...);
		}

		template<typename TBuilder, typename THooks, typename... TArgs>
		static auto Build(const THooks& hooks, TArgs&&... args) {
			TBuilder builder;
			ApplyAll(builder, hooks);
			return builder.build(std::forward<TArgs>(args)...);
		}
	}

	// region handlers

	void PluginManager::addHandlerHook(const HandlerHook& hook) {
		m_nonDiagnosticHandlerHooks.push_back(hook);
	}

	void PluginManager::addHandlers(ionet::ServerPacketHandlers& handlers, const cache::CatapultCache& cache) const {
		ApplyAll(handlers, m_nonDiagnosticHandlerHooks, cache);
	}

	// endregion

	// region diagnostics

	void PluginManager::addDiagnosticHandlerHook(const HandlerHook& hook) {
		m_diagnosticHandlerHooks.push_back(hook);
	}

	void PluginManager::addDiagnosticCounterHook(const CounterHook& hook) {
		m_diagnosticCounterHooks.push_back(hook);
	}

	void PluginManager::addDiagnosticHandlers(ionet::ServerPacketHandlers& handlers, const cache::CatapultCache& cache) const {
		ApplyAll(handlers, m_diagnosticHandlerHooks, cache);
	}

	void PluginManager::addDiagnosticCounters(std::vector<utils::DiagnosticCounter>& counters, const cache::CatapultCache& cache) const {
		ApplyAll(counters, m_diagnosticCounterHooks, cache);
	}

	// endregion

	// region validators

	void PluginManager::addStatelessValidatorHook(const StatelessValidatorHook& hook) {
		m_statelessValidatorHooks.push_back(hook);
	}

	void PluginManager::addStatefulValidatorHook(const StatefulValidatorHook& hook) {
		m_statefulValidatorHooks.push_back(hook);
	}

	PluginManager::StatelessValidatorPointer PluginManager::createStatelessValidator(
			const validators::ValidationResultPredicate& isSuppressedFailure) const {
		return Build<validators::stateless::DemuxValidatorBuilder>(m_statelessValidatorHooks, isSuppressedFailure);
	}

	PluginManager::StatelessValidatorPointer PluginManager::createStatelessValidator() const {
		return createStatelessValidator([](auto) { return false; });
	}

	PluginManager::StatefulValidatorPointer PluginManager::createStatefulValidator(
			const validators::ValidationResultPredicate& isSuppressedFailure) const {
		return Build<validators::stateful::DemuxValidatorBuilder>(m_statefulValidatorHooks, isSuppressedFailure);
	}

	PluginManager::StatefulValidatorPointer PluginManager::createStatefulValidator() const {
		return createStatefulValidator([](auto) { return false; });
	}

	// endregion

	// region observers

	void PluginManager::addObserverHook(const ObserverHook& hook) {
		m_observerHooks.push_back(hook);
	}

	void PluginManager::addTransientObserverHook(const ObserverHook& hook) {
		m_transientObserverHooks.push_back(hook);
	}

	PluginManager::ObserverPointer PluginManager::createObserver() const {
		observers::DemuxObserverBuilder builder;
		ApplyAll(builder, m_observerHooks);
		ApplyAll(builder, m_transientObserverHooks);
		return builder.build();
	}

	PluginManager::ObserverPointer PluginManager::createPermanentObserver() const {
		return Build<observers::DemuxObserverBuilder>(m_observerHooks);
	}

	// endregion

	// region resolvers

	void PluginManager::addMosaicResolver(const MosaicResolver& resolver) {
		m_mosaicResolvers.push_back(resolver);
	}

	void PluginManager::addAddressResolver(const AddressResolver& resolver) {
		m_addressResolvers.push_back(resolver);
	}

	void PluginManager::addAmountResolver(const AmountResolver& resolver) {
		m_amountResolvers.push_back(resolver);
	}

	namespace {
		template<typename TResolved, typename TResolvers>
		auto BuildResolver(const TResolvers& resolvers) {
			return [resolvers](const auto& cache, const auto& unresolved) {
				TResolved resolved;
				for (const auto& resolver : resolvers) {
					if (resolver(cache, unresolved, resolved))
						return resolved;
				}

				return model::ResolverContext().resolve(unresolved);
			};
		}
	}

	model::ResolverContext PluginManager::createResolverContext(const cache::ReadOnlyCatapultCache& cache) const {
		auto mosaicResolver = BuildResolver<MosaicId>(m_mosaicResolvers);
		auto addressResolver = BuildResolver<Address>(m_addressResolvers);
		auto amountResolver = BuildResolver<Amount>(m_amountResolvers);

		auto bindResolverToCache = [&cache](const auto& resolver) {
			return [&cache, resolver](const auto& unresolved) { return resolver(cache, unresolved); };
		};

		return model::ResolverContext(bindResolverToCache(mosaicResolver), bindResolverToCache(addressResolver), bindResolverToCache(amountResolver));
	}

	void PluginManager::addAddressesExtractor(const AddressesExtractor& extractor) {
		m_addressesExtractors.push_back(extractor);
	}

	void PluginManager::addPublicKeysExtractor(const PublicKeysExtractor& extractor) {
		m_publicKeysExtractors.push_back(extractor);
	}

	namespace {
		template<typename TExtractors>
		auto BuildExtractor(const TExtractors& extractors) {
			return [extractors](const auto& cache, const auto& address) {
				auto result = model::ExtractorContext().extract(address);

				for (const auto& extractor : extractors) {
					auto extractedAddresses = extractor(cache, address);
					result.insert(extractedAddresses.begin(), extractedAddresses.end());
				}

				return result;
			};
		}
	}

	model::ExtractorContext PluginManager::createExtractorContext(const cache::CatapultCache& cache) const {
		auto addressesExtractors = BuildExtractor(m_addressesExtractors);
		auto publicKeysExtractors = BuildExtractor(m_publicKeysExtractors);

		auto bindExtractorToCache = [&cache](const auto& extractor) {
			return [&cache, extractor](const auto& address) {
				auto view = cache.createView();
				return extractor(view.toReadOnly(), address);
			};
		};

		return model::ExtractorContext(bindExtractorToCache(addressesExtractors), bindExtractorToCache(publicKeysExtractors));
	}

	void PluginManager::addPluginInitializer(PluginInitializer&& initializer) {
		m_pluginInitializers.push_back(std::move(initializer));
	}

	PluginManager::PluginInitializer PluginManager::createPluginInitializer() const {
        return [initializers = m_pluginInitializers](auto& config) {
            for (const auto& init : initializers)
                init(config);
        };
	}

	// endregion

	// region publisher

	PluginManager::PublisherPointer PluginManager::createNotificationPublisher(model::PublicationMode mode) const {
		return model::CreateNotificationPublisher(m_transactionRegistry,
												  config::GetUnresolvedCurrencyMosaicId(immutableConfig()),
												  *m_pTransactionFeeCalculator,
												  mode);
	}

	// endregion

	// region committee

	/// Sets a committee manager.
	void PluginManager::setCommitteeManager(const std::shared_ptr<chain::CommitteeManager>& pManager) {
		if (!!m_pCommitteeManager)
			CATAPULT_THROW_RUNTIME_ERROR("committee manager already set");

		m_pCommitteeManager = pManager;
	}

	/// Gets committee manager.
	chain::CommitteeManager& PluginManager::getCommitteeManager() const {
		if (!m_pCommitteeManager)
			CATAPULT_THROW_RUNTIME_ERROR("committee manager not set");

		return *m_pCommitteeManager;
	}

	// endregion

	// region dbrb

	/// Sets a DBRB view fetcher.
	void PluginManager::setDbrbViewFetcher(const std::shared_ptr<dbrb::DbrbViewFetcher>& pFetcher) {
		if (!!m_pDbrbViewFetcher)
			CATAPULT_THROW_RUNTIME_ERROR("DBRB view fetcher already set");

		m_pDbrbViewFetcher = pFetcher;
	}

	/// Gets DBRB view fetcher.
	dbrb::DbrbViewFetcher& PluginManager::dbrbViewFetcher() const {
		if (!m_pDbrbViewFetcher)
			CATAPULT_THROW_RUNTIME_ERROR("DBRB view fetcher not set");

		return *m_pDbrbViewFetcher;
	}

	// endregion

	// region storage

	void PluginManager::setStorageState(const std::shared_ptr<state::StorageState>& pState) {
		if (!!m_pStorageState)
			CATAPULT_THROW_RUNTIME_ERROR("storage state already set");

		m_pStorageState = pState;
	}

	bool PluginManager::isStorageStateSet() {
		return !!m_pStorageState;
	}

	state::StorageState& PluginManager::storageState() const {
		if (!m_pStorageState)
			CATAPULT_THROW_RUNTIME_ERROR("storage state not set");

		return *m_pStorageState;
	}

	// endregion

	// region liquidity provider
	void PluginManager::setLiquidityProviderExchangeValidator(
			std::unique_ptr<validators::LiquidityProviderExchangeValidator>&& validator) {
		m_pLiquidityProviderExchangeValidator = std::move(validator);
	}

	const std::unique_ptr<validators::LiquidityProviderExchangeValidator>& PluginManager::liquidityProviderExchangeValidator() const {
		return m_pLiquidityProviderExchangeValidator;
	}

	void PluginManager::setLiquidityProviderExchangeObserver(std::unique_ptr<observers::LiquidityProviderExchangeObserver>&& observer) {
		m_pLiquidityProviderExchangeObserver = std::move(observer);
	}

	const std::unique_ptr<observers::LiquidityProviderExchangeObserver>& PluginManager::liquidityProviderExchangeObserver() const {
		return m_pLiquidityProviderExchangeObserver;
	}

	// endregion

	// region storage updates listeners

	const std::vector<std::unique_ptr<observers::StorageUpdatesListener>>&
	PluginManager::storageUpdatesListeners() const {
		return m_storageUpdatesListeners;
	}
	void PluginManager::addStorageUpdateListener(std::unique_ptr<observers::StorageUpdatesListener>&& storageUpdatesListener) {
		m_storageUpdatesListeners.emplace_back(std::move(storageUpdatesListener));
	}

	// endregion

	// region transaction fee limiter

	std::shared_ptr<model::TransactionFeeCalculator> PluginManager::transactionFeeCalculator() const {
		return m_pTransactionFeeCalculator;
	}

	// endregion

	// region configure plugin manager

	void PluginManager::reset() {
		m_transactionRegistry.reset();
		m_nonDiagnosticHandlerHooks.clear();
		m_diagnosticHandlerHooks.clear();
		m_diagnosticCounterHooks.clear();
		m_statelessValidatorHooks.clear();
		m_statefulValidatorHooks.clear();
		m_observerHooks.clear();
		m_transientObserverHooks.clear();
		m_mosaicResolvers.clear();
		m_addressResolvers.clear();
		m_amountResolvers.clear();
		m_addressesExtractors.clear();
		m_publicKeysExtractors.clear();
		m_pluginInitializers.clear();
	}

	// endregion
}}
