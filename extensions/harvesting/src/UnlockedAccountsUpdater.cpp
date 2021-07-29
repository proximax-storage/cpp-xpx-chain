/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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

#include "UnlockedAccountsUpdater.h"
#include "UnlockedAccounts.h"
#include "UnlockedFileQueueConsumer.h"
#include "catapult/cache/CatapultCache.h"
#include "catapult/cache_core/ImportanceView.h"
#include "catapult/io/RawFile.h"
#include "catapult/model/Address.h"
#include "catapult/utils/ThrottleLogger.h"

namespace catapult { namespace harvesting {

	namespace {
		bool AddToUnlocked(UnlockedAccounts& unlockedAccounts, crypto::KeyPair&& descriptor) {
			auto signingPublicKey = descriptor.publicKey();
			auto addResult = unlockedAccounts.modifier().add(std::move(descriptor));
			if (UnlockedAccountsAddResult::Success_New == addResult) {
				CATAPULT_LOG(info) << "added NEW account " << signingPublicKey;
				return true;
			}

			return false;
		}

		bool RemoveFromUnlocked(UnlockedAccounts& unlockedAccounts, const Key& publicKey) {
			if (unlockedAccounts.modifier().remove(publicKey)) {
				CATAPULT_LOG(info) << "removed account " << publicKey;
				return true;
			}

			return false;
		}

		// region DescriptorProcessor

		class DescriptorProcessor {
		public:
			DescriptorProcessor(
					const Key& signingPublicKey,
					const Key& nodePublicKey,
					const cache::CatapultCache& cache,
					UnlockedAccounts& unlockedAccounts,
					const std::shared_ptr<config::BlockchainConfigurationHolder>& configHolder,
					UnlockedAccountsStorage& storage)
					: m_signingPublicKey(signingPublicKey)
					, m_ConfigHolder(configHolder)
					, m_nodePublicKey(nodePublicKey)
					, m_cache(cache)
					, m_unlockedAccounts(unlockedAccounts)
					, m_storage(storage)
					, m_hasAnyRemoval(false)
			{}

		public:
			bool hasAnyRemoval() const {
				return m_hasAnyRemoval;
			}

		public:
			void operator()(const HarvestRequest& harvestRequest, crypto::KeyPair&& descriptor) {
				if (HarvestRequestOperation::Add == harvestRequest.Operation)
					add(harvestRequest, std::move(descriptor));
				else
					remove(GetRequestIdentifier(harvestRequest), descriptor.publicKey());
			}

			size_t pruneUnlockedAccounts() {
				auto cacheView = m_cache.createView();

				size_t numPrunedAccounts = 0;
				auto height = cacheView.height() + Height(1);
				m_unlockedAccounts.modifier().removeIf([this, height, &cacheView, &numPrunedAccounts](const auto& descriptor) {

					auto readOnlyAccountStateCache = cache::ReadOnlyAccountStateCache(cacheView.sub<cache::AccountStateCache>());
					cache::ImportanceView view(readOnlyAccountStateCache);

					auto address = model::PublicKeyToAddress(descriptor, readOnlyAccountStateCache.networkIdentifier());
					auto minHarvesterBalance = m_ConfigHolder->Config(height).Network.MinHarvesterBalance;
					auto shouldPruneAccount = !view.canHarvest(descriptor, height, minHarvesterBalance );

					if (shouldPruneAccount && m_signingPublicKey == descriptor) {
						CATAPULT_LOG_THROTTLE(warning, utils::TimeSpan::FromHours(6).millis())
								<< "primary signing public key " << m_signingPublicKey << " does not meet harvesting requirements";
						return false;
					}

					if (!shouldPruneAccount) {
						auto remoteAccountStateIter = readOnlyAccountStateCache.find(address);
						if (state::AccountType::Remote == remoteAccountStateIter.get().AccountType) {
							auto mainAccountStateIter = readOnlyAccountStateCache.find(GetLinkedPublicKey(remoteAccountStateIter.get()));
							shouldPruneAccount = !isMainAccountEligibleForDelegation(mainAccountStateIter.get(), descriptor);
						}
					}

					if (shouldPruneAccount)
						++numPrunedAccounts;

					return shouldPruneAccount;
				});

				return numPrunedAccounts;
			}

		private:
			void add(const HarvestRequest& harvestRequest, crypto::KeyPair&& descriptor) {
				if (!isMainAccountEligibleForDelegation(harvestRequest.MainAccountPublicKey, descriptor.publicKey()))
					return;

				auto harvesterSigningPublicKey = descriptor.publicKey();
				auto requestIdentifier = GetRequestIdentifier(harvestRequest);
				if (!m_storage.contains(requestIdentifier) && AddToUnlocked(m_unlockedAccounts, std::move(descriptor)))
					m_storage.add(requestIdentifier, harvestRequest.EncryptedPayload, harvesterSigningPublicKey);
			}

			void remove(HarvestRequestIdentifier requestIdentifier, const Key& harvesterSigningPublicKey) {
				RemoveFromUnlocked(m_unlockedAccounts, harvesterSigningPublicKey);
				m_storage.remove(requestIdentifier);
				m_hasAnyRemoval = true;
			}

			bool isMainAccountEligibleForDelegation(const Key& signingAccountPublicKey, const Key& descriptor) {
				auto cacheView = m_cache.createView();
				auto readOnlyAccountStateCache = cache::ReadOnlyAccountStateCache(cacheView.sub<cache::AccountStateCache>());
				auto remoteAccountStateIter = readOnlyAccountStateCache.find(descriptor);
				if (!remoteAccountStateIter.tryGet()) {
					CATAPULT_LOG(warning) << "rejecting delegation from " << signingAccountPublicKey << ": unknown remote account";
					return false;
				}
				auto remoteAccountState = remoteAccountStateIter.get();
				auto mainAccountStateIter = readOnlyAccountStateCache.find(GetLinkedPublicKey(remoteAccountState));
				if (!mainAccountStateIter.tryGet()) {
					CATAPULT_LOG(warning) << "rejecting delegation from " << signingAccountPublicKey << ": unknown main account";
					return false;
				}
				auto signingAccountStateIter = readOnlyAccountStateCache.find(signingAccountPublicKey);
				if (!signingAccountStateIter.tryGet()) {
					CATAPULT_LOG(warning) << "rejecting delegation from " << signingAccountPublicKey << ": unknown signing account";
					return false;
				}
				return isMainAccountEligibleForDelegation(mainAccountStateIter.get(), descriptor);
			}

			bool isMainAccountEligibleForDelegation(
					const state::AccountState& mainAccountState,
					const Key& descriptor) {
				if (GetLinkedPublicKey(mainAccountState) != descriptor) {
					CATAPULT_LOG(warning) << "rejecting delegation from " << mainAccountState.PublicKey << ": invalid main account linking." << GetLinkedPublicKey(mainAccountState);
					return false;
				}

				// skip node link check if main account has signed this persistent harvesting delegation request(meaning he authorizes it?)
				if (GetLinkedPublicKey(mainAccountState) == m_signingPublicKey)
					return true;

				if (GetNodePublicKey(mainAccountState) != m_nodePublicKey) {
					CATAPULT_LOG(warning) << "rejecting delegation from " << mainAccountState.PublicKey << ": invalid node public key." << GetNodePublicKey(mainAccountState);
					return false;
				}

				return true;
			}

		private:
			Key m_signingPublicKey;
			Key m_nodePublicKey;
			const cache::CatapultCache& m_cache;
			UnlockedAccounts& m_unlockedAccounts;
			const std::shared_ptr<config::BlockchainConfigurationHolder>& m_ConfigHolder;
			UnlockedAccountsStorage& m_storage;
			bool m_hasAnyRemoval;
		};

		// endregion
	}

	UnlockedAccountsUpdater::UnlockedAccountsUpdater(
			const cache::CatapultCache& cache,
			UnlockedAccounts& unlockedAccounts,
			const Key& signingPublicKey,
			const crypto::KeyPair& encryptionKeyPair,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& configHolder,
			const config::CatapultDataDirectory& dataDirectory)
			: m_cache(cache)
			, m_unlockedAccounts(unlockedAccounts)
			, m_signingPublicKey(signingPublicKey)
			, m_encryptionKeyPair(encryptionKeyPair)
			, m_dataDirectory(dataDirectory)
			, m_configHolder(configHolder)
			, m_harvestersFilename(m_dataDirectory.rootDir().file("harvesters.dat"))
			, m_unlockedAccountsStorage(m_harvestersFilename)
	{}

	void UnlockedAccountsUpdater::load() {
		// load account descriptors
		m_unlockedAccountsStorage.load(m_encryptionKeyPair, [&unlockedAccounts = m_unlockedAccounts](auto&& descriptor) {
			AddToUnlocked(unlockedAccounts, std::move(descriptor));
		});
	}

	void UnlockedAccountsUpdater::update() {
		// 1. process queued accounts
		DescriptorProcessor processor(
				m_signingPublicKey,
				m_encryptionKeyPair.publicKey(),
				m_cache,
				m_unlockedAccounts,
				m_configHolder,
				m_unlockedAccountsStorage);

		auto cacheHeight = m_cache.createView().height();
		UnlockedFileQueueConsumer(m_dataDirectory.dir("transfer_message"), cacheHeight, m_encryptionKeyPair, std::ref(processor));

		// 2. prune accounts that are not eligible to harvest the next block
		auto numPrunedAccounts = processor.pruneUnlockedAccounts();

		// 3. save accounts
		if (numPrunedAccounts > 0 || processor.hasAnyRemoval()) {
			auto view = m_unlockedAccounts.view();
			m_unlockedAccountsStorage.save([&view](const auto& harvesterSigningPublicKey) {
				return view.contains(harvesterSigningPublicKey);
			});
		}
	}
}}
