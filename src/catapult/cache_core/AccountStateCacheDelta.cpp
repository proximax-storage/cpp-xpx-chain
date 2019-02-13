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

#include "AccountStateCacheDelta.h"
#include "catapult/model/Address.h"
#include "catapult/utils/Casting.h"
#include "catapult/utils/HexFormatter.h"
#include "catapult/functions.h"

namespace catapult { namespace cache {

	BasicAccountStateCacheDelta::BasicAccountStateCacheDelta(
			const AccountStateCacheTypes::BaseSetDeltaPointers& accountStateSets,
			const AccountStateCacheTypes::Options& options,
			const model::AddressSet& highValueAddresses,
			const model::AddressSet& addressesToUpdate)
			: BasicAccountStateCacheDelta(
					accountStateSets,
					options,
					highValueAddresses,
					addressesToUpdate,
					std::make_unique<AccountStateCacheDeltaMixins::KeyLookupAdapter>(
							*accountStateSets.pKeyLookupMap,
							*accountStateSets.pPrimary))
	{}

	BasicAccountStateCacheDelta::BasicAccountStateCacheDelta(
			const AccountStateCacheTypes::BaseSetDeltaPointers& accountStateSets,
			const AccountStateCacheTypes::Options& options,
			const model::AddressSet& highValueAddresses,
			const model::AddressSet& addressesToUpdate,
			std::unique_ptr<AccountStateCacheDeltaMixins::KeyLookupAdapter>&& pKeyLookupAdapter)
			: AccountStateCacheDeltaMixins::Size(*accountStateSets.pPrimary)
			, AccountStateCacheDeltaMixins::ContainsAddress(*accountStateSets.pPrimary)
			, AccountStateCacheDeltaMixins::ContainsKey(*accountStateSets.pKeyLookupMap)
			, AccountStateCacheDeltaMixins::ConstAccessorAddress(*accountStateSets.pPrimary)
			, AccountStateCacheDeltaMixins::ConstAccessorKey(*pKeyLookupAdapter)
			, AccountStateCacheDeltaMixins::MutableAccessorAddress(*accountStateSets.pPrimary)
			, AccountStateCacheDeltaMixins::MutableAccessorKey(*pKeyLookupAdapter)
			, AccountStateCacheDeltaMixins::PatriciaTreeDelta(*accountStateSets.pPrimary, accountStateSets.pPatriciaTree)
			, AccountStateCacheDeltaMixins::DeltaElements(*accountStateSets.pPrimary)
			, m_pStateByAddress(accountStateSets.pPrimary)
			, m_pKeyToAddress(accountStateSets.pKeyLookupMap)
			, m_options(options)
			, m_highValueAddresses(highValueAddresses)
			, m_addressesToUpdate(addressesToUpdate)
			, m_pKeyLookupAdapter(std::move(pKeyLookupAdapter))
	{}

	model::NetworkIdentifier BasicAccountStateCacheDelta::networkIdentifier() const {
		return m_options.NetworkIdentifier;
	}

	uint64_t BasicAccountStateCacheDelta::importanceGrouping() const {
		return m_options.ImportanceGrouping;
	}

	MosaicId BasicAccountStateCacheDelta::harvestingMosaicId() const {
		return m_options.HarvestingMosaicId;
	}

	Address BasicAccountStateCacheDelta::getAddress(const Key& publicKey) {
		auto keyToAddressIter = m_pKeyToAddress->find(publicKey);
		const auto* pPair = keyToAddressIter.get();
		if (pPair)
			return pPair->second;

		auto address = model::PublicKeyToAddress(publicKey, m_options.NetworkIdentifier);
		m_pKeyToAddress->emplace(publicKey, address);
		return address;
	}

	void BasicAccountStateCacheDelta::addAccount(const Address& address, Height height) {
		if (contains(address))
			return;

		addAccount(state::AccountState(address, height));
	}

	void BasicAccountStateCacheDelta::addAccount(const Key& publicKey, Height height) {
		auto address = getAddress(publicKey);
		addAccount(address, height);

		// optimize common case where public key is already known by not marking account as dirty in that case
		auto accountStateIterConst = const_cast<const BasicAccountStateCacheDelta*>(this)->find(address);
		auto& accountStateConst = accountStateIterConst.get();
		if (Height(0) != accountStateConst.PublicKeyHeight)
			return;

		auto accountStateIter = this->find(address);
		auto& accountState = accountStateIter.get();
		accountState.PublicKey = publicKey;
		accountState.PublicKeyHeight = height;
	}

	void BasicAccountStateCacheDelta::addAccount(const state::AccountState& accountState) {
		if (contains(accountState.Address))
			return;

		if (Height(0) != accountState.PublicKeyHeight)
			m_pKeyToAddress->emplace(accountState.PublicKey, accountState.Address);

		m_pStateByAddress->insert(accountState);
		m_pStateByAddress->find(accountState.Address).get()->Balances.optimize(m_options.CurrencyMosaicId);
		m_pStateByAddress->find(accountState.Address).get()->Balances.track(m_options.HarvestingMosaicId);
	}

	void BasicAccountStateCacheDelta::remove(const Address& address, Height height) {
		auto accountStateIter = this->find(address);
		if (!accountStateIter.tryGet())
			return;

		const auto& accountState = accountStateIter.get();
		if (height != accountState.AddressHeight)
			return;

		// note: we can only remove the entry from m_pKeyToAddress if the account state's public key is valid
		if (Height(0) != accountState.PublicKeyHeight)
			m_pKeyToAddress->remove(accountState.PublicKey);

		m_pStateByAddress->remove(address);
	}

	void BasicAccountStateCacheDelta::remove(const Key& publicKey, Height height) {
		auto accountStateIter = this->find(publicKey);
		if (!accountStateIter.tryGet())
			return;

		auto& accountState = accountStateIter.get();
		if (height != accountState.PublicKeyHeight)
			return;

		m_pKeyToAddress->remove(accountState.PublicKey);

		// if same height, remove address entry too
		if (accountState.PublicKeyHeight == accountState.AddressHeight) {
			m_pStateByAddress->remove(accountState.Address);
			return;
		}

		// safe, as the account is still in m_pStateByAddress
		accountState.PublicKeyHeight = Height(0);
		accountState.PublicKey = Key{};
	}

	void BasicAccountStateCacheDelta::queueRemove(const Address& address, Height height) {
		m_queuedRemoveByAddress.emplace(height, address);
	}

	void BasicAccountStateCacheDelta::queueRemove(const Key& publicKey, Height height) {
		m_queuedRemoveByPublicKey.emplace(height, publicKey);
	}

	void BasicAccountStateCacheDelta::commitRemovals() {
		for (const auto& addressHeightPair : m_queuedRemoveByAddress)
			remove(addressHeightPair.second, addressHeightPair.first);

		for (const auto& keyHeightPair : m_queuedRemoveByPublicKey)
			remove(keyHeightPair.second, keyHeightPair.first);

		m_queuedRemoveByAddress.clear();
		m_queuedRemoveByPublicKey.clear();
	}

	namespace {
		using DeltasSet = AccountStateCacheTypes::PrimaryTypes::BaseSetDeltaType::SetType::MemorySetType;

		void UpdateAddresses(model::AddressSet& addresses, const DeltasSet& source, const predicate<const state::AccountState&>& include) {
			for (const auto& pair : source) {
				const auto& accountState = pair.second;
				if (include(accountState))
					addresses.insert(accountState.Address);
				else
					addresses.erase(accountState.Address);
			}
		}

		void commitSnapshotsOf(const DeltasSet& source) {
			for (auto& pairs : source) {
				// TODO: re-work it in future
				const_cast<state::AccountState&>(pairs.second).Balances.commitSnapshots();
			}
		}
	}

	void BasicAccountStateCacheDelta::commitSnapshots() const {
		auto deltas = m_pStateByAddress->deltas();
		commitSnapshotsOf(deltas.Added);
		commitSnapshotsOf(deltas.Copied);
	}

	void BasicAccountStateCacheDelta::addUpdatedAddresses(model::AddressSet& set) const {
		auto include = [](const auto& accountState) {
			return !accountState.Balances.snapshots().empty();
		};

		auto deltas = m_pStateByAddress->deltas();
		UpdateAddresses(set, deltas.Added, include);
		UpdateAddresses(set, deltas.Copied, include);
		UpdateAddresses(set, deltas.Removed, [](const auto&) { return false; });
	}

	model::AddressSet BasicAccountStateCacheDelta::highValueAddresses() const {
		// 1. copy original high value addresses
		auto highValueAddresses = m_highValueAddresses;

		// 2. update for changes
		auto minBalance = m_options.MinHighValueAccountBalance;
		auto harvestingMosaicId = m_options.HarvestingMosaicId;
		auto hasHighValue = [minBalance, harvestingMosaicId](const auto& accountState) {
			return accountState.Balances.get(harvestingMosaicId) >= minBalance;
		};

		auto deltas = m_pStateByAddress->deltas();
		UpdateAddresses(highValueAddresses, deltas.Added, hasHighValue);
		UpdateAddresses(highValueAddresses, deltas.Copied, hasHighValue);
		UpdateAddresses(highValueAddresses, deltas.Removed, [](const auto&) { return false; });
		return highValueAddresses;
	}

	const model::AddressSet& BasicAccountStateCacheDelta::updatedAddresses() const{
		return m_addressesToUpdate;
	}
}}
