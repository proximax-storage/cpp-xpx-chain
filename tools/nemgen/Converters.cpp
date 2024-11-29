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

#include "Converters.h"
#include "catapult/utils/HexParser.h"
#include "catapult/config/ModifyStateConfiguration.h"
#include <regex>

namespace catapult { namespace tools { namespace nemgen {

	state::NetworkConfigEntry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::NetworkConfigEntry& value, bool is_future) {
		std::istringstream inputBlock(value.networkConfig());
		auto networkConfig = model::NetworkConfiguration::LoadFromBag(utils::ConfigurationBag::FromStream(inputBlock));
		auto nemesisKey = accountMigrationManager.GetKeyPair(networkConfig.Info.PublicKey)->publicKey();
		std::string newKey;
		std::string oldKey;
		utils::ParseHexStringIntoContainer(
				reinterpret_cast<char* >(nemesisKey.data()), Key_Size, newKey);
		utils::ParseHexStringIntoContainer(
				reinterpret_cast<char* >(networkConfig.Info.PublicKey.data()), Key_Size, oldKey);
		auto newConfig = std::regex_replace(value.networkConfig(), std::regex(oldKey), newKey);
		if(!networkConfig.GetPluginConfiguration<config::ModifyStateConfiguration>().Enabled) {
			//TODO update future configurations with proper plugin section
		}
		return state::NetworkConfigEntry(value.height(), newConfig, value.supportedEntityVersions());
	}

	state::AccountState Convert(utils::AccountMigrationManager& accountMigrationManager, const state::AccountState& value) {
		state::AccountState result = value;
		result.Address = accountMigrationManager.GetActiveAddress(value.Address);
		result.LinkedAccountKey = accountMigrationManager.GetKeyPair(value.LinkedAccountKey)->publicKey();
		result.PublicKey = accountMigrationManager.GetKeyPair(result.Address)->publicKey();
		return result;
	}

	state::ExchangeEntry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::ExchangeEntry& value) {
		return state::ExchangeEntry(value, accountMigrationManager.GetKeyPair(value.owner())->publicKey());
	}

	state::MetadataV1Entry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::MetadataV1Entry& value) {
		if(value.type() == model::MetadataV1Type::NamespaceId || value.type() == model::MetadataV1Type::MosaicId) {
			return value;
		}
		auto vec = value.raw();
		Address address;
		memcpy(address.data(), vec.data(), vec.size());
		state::MetadataV1Entry result(state::ToVector(accountMigrationManager.GetActiveAddress(address)), value.type());
		result.fields() = value.fields();
		return result;
	}

	state::MetadataEntry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::MetadataEntry& value) {
		auto partialMetadataKey = model::PartialMetadataKey{
			accountMigrationManager.GetActiveAddress(value.key().sourceAddress()),
			accountMigrationManager.GetKeyPair(value.key().targetKey())->publicKey(),
			value.key().scopedMetadataKey()
		};
		switch(value.key().metadataType()) {
		case model::MetadataType::Account:
		{
			auto entry = state::MetadataEntry(state::MetadataKey(partialMetadataKey));
			entry.value() = value.value();
			return entry;
		}
		case model::MetadataType::Mosaic:
		{
			auto entry = state::MetadataEntry(state::MetadataKey(partialMetadataKey, value.key().mosaicTarget()));
			entry.value() = value.value();
			return entry;
		}
		case model::MetadataType::Namespace:
		{
			auto entry = state::MetadataEntry(state::MetadataKey(partialMetadataKey,value.key().namespaceTarget()));
			entry.value() = value.value();
			return entry;
		}
		}
	}

	state::MosaicEntry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::MosaicEntry& value) {
		auto definition = state::MosaicDefinition(value.definition().height(), accountMigrationManager.GetKeyPair(value.definition().owner())->publicKey(), value.definition().revision(), value.definition().properties());
		return state::MosaicEntry(value.mosaicId(), definition);
	}

	state::LevyEntry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::LevyEntry& value) {
		auto entryData = state::LevyEntryData(value.levy()->Type, accountMigrationManager.GetActiveAddress(value.levy()->Recipient), value.levy()->MosaicId, value.levy()->Fee);
		auto entry = state::LevyEntry(value.mosaicId(), entryData);
		for(auto pair : value.updateHistory()) {
			auto historyEntry = state::LevyEntryData(pair.second.Type, accountMigrationManager.GetActiveAddress(pair.second.Recipient), pair.second.MosaicId, pair.second.Fee);
			entry.updateHistory().push_back(std::make_pair(pair.first, historyEntry));
		}
		return entry;
	}

	state::MultisigEntry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::MultisigEntry& value) {
		auto multisigEntry = state::MultisigEntry(accountMigrationManager.GetKeyPair(value.key())->publicKey());
		for(auto key : value.cosignatories()) {
			multisigEntry.cosignatories().insert(accountMigrationManager.GetKeyPair(key)->publicKey());
		}
		for(auto key : value.multisigAccounts()) {
			multisigEntry.multisigAccounts().insert(accountMigrationManager.GetKeyPair(key)->publicKey());
		}
		return multisigEntry;
	}

	state::RootNamespaceHistory Convert(utils::AccountMigrationManager& accountMigrationManager, const state::RootNamespaceHistory& value) {
		auto history = state::RootNamespaceHistory(value.id());
		for(auto historyEntry : value) {
			history.push_back(accountMigrationManager.GetKeyPair(historyEntry.owner())->publicKey(), historyEntry.lifetime());
			auto& current = history.back();
			current = state::RootNamespace(historyEntry.id(), accountMigrationManager.GetKeyPair(historyEntry.owner())->publicKey(), historyEntry.lifetime(), std::make_shared<state::RootNamespace::Children>(historyEntry.children()));
			auto alias = historyEntry.alias(historyEntry.id());
			if(alias.type() == state::AliasType::Address)
				current.setAlias(historyEntry.id(), state::NamespaceAlias(accountMigrationManager.GetActiveAddress(alias.address())));
			else
				current.setAlias(historyEntry.id(), alias);
		}
		return history;
	}

	state::AccountProperties Convert(utils::AccountMigrationManager& accountMigrationManager, const state::AccountProperties& value) {
		auto result = state::AccountProperties(accountMigrationManager.GetActiveAddress(value.address()));
		result.property(model::PropertyType::MosaicId) = value.property(model::PropertyType::MosaicId);
		result.property(model::PropertyType::TransactionType) = value.property(model::PropertyType::TransactionType);
		const state::AccountProperty& addressProperty = value.property(model::PropertyType::Address);
		for(const auto& val : addressProperty.values()) {
			Address address;
			memcpy(address.data(), val.data(), val.size());
			if(addressProperty.descriptor().operationType() == state::OperationType::Allow)
				result.property(model::PropertyType::Address).allow(model::RawPropertyModification{
						model::PropertyModificationType::Add,
						utils::ToVector(accountMigrationManager.GetActiveAddress(address)),
				});
			else result.property(model::PropertyType::Address).block(model::RawPropertyModification{
						model::PropertyModificationType::Add,
						utils::ToVector(accountMigrationManager.GetActiveAddress(address)),
				});
		}
		return result;
	}
	state::BcDriveEntry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::BcDriveEntry& value) {
		auto bcEntry = state::BcDriveEntry(value.key());
		bcEntry.setVersion(value.version());
		bcEntry.setOwner(accountMigrationManager.GetKeyPair(value.owner())->publicKey());

		for(auto replicator : value.replicators()) {
			bcEntry.replicators().insert(accountMigrationManager.GetKeyPair(replicator)->publicKey());
		}

		for(auto offReplicator : value.offboardingReplicators()) {
			bcEntry.offboardingReplicators().insert(accountMigrationManager.GetKeyPair(offReplicator)->publicKey());
		}

		bcEntry.setSize(value.size());
		bcEntry.setReplicatorCount(value.replicatorCount());
		bcEntry.setLastPayment(value.getLastPayment());

		return bcEntry;
	}

	state::CommitteeEntry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::CommitteeEntry& value) {
		return state::CommitteeEntry(
						accountMigrationManager.GetKeyPair(value.key())->publicKey(),
						accountMigrationManager.GetKeyPair(value.owner())->publicKey(),
						value.lastSigningBlockHeight(),
						value.effectiveBalance(),
						value.canHarvest(),
						value.activity(),
						value.greed(),
						value.disabledHeight());
	}
}}}
