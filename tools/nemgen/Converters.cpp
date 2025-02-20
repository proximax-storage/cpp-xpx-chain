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
#include "catapult/model/ModifyStateEntityType.h"
#include "catapult/config/SupportedEntityVersions.h"
#include <iostream>
#include <sstream>
#include <string>
#include <regex>

namespace catapult { namespace tools { namespace nemgen {

	namespace {
		std::string replaceOrAddBlockInINIString(const std::string& iniContent, const std::string& blockHeader, const std::string& newBlockContent) {
			// Regular expression to match the block starting with the given header
			std::regex blockRegex("\\[" + blockHeader + "](.*?)(?=(\\[.*\\])|\\z)", std::regex_constants::ECMAScript);

			if (std::regex_search(iniContent, blockRegex)) {
				// Replace the matched block with the new content
				return std::regex_replace(iniContent, blockRegex, "[" + blockHeader + "]\n" + newBlockContent + "\n");
			} else {
				// Append the new block at the end if it doesn't exist
				return iniContent + "\n[" + blockHeader + "]\n" + newBlockContent + "\n";
			}
		}
	}
	state::NetworkConfigEntry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::NetworkConfigEntry& value, bool isFuture) {
		std::istringstream inputBlock(value.networkConfig());
		auto networkConfig = model::NetworkConfiguration::LoadFromBag(utils::ConfigurationBag::FromStream(inputBlock));
		accountMigrationManager.AssociateNemesis(networkConfig.Info.PublicKey);
		auto nemesisKey = accountMigrationManager.GetKeyPair(networkConfig.Info.PublicKey)->publicKey();
		std::ostringstream sNewKey;
		std::ostringstream sOldKey;
		sNewKey << nemesisKey;
		sOldKey << networkConfig.Info.PublicKey;

		auto newConfig = std::regex_replace(value.networkConfig(), std::regex(sOldKey.str()), sNewKey.str());
		auto newSupportedEntities = value.supportedEntityVersions();
		/// If is current or future network config, force enable modify state plugin
		if(isFuture) {
			std::string stateBlock = ""; // Your INI string
			stateBlock += "[plugin:catapult.plugins.modifystate]\n";
			stateBlock += "enabled=true\n";
			newConfig = replaceOrAddBlockInINIString(newConfig, "plugin:catapult.plugins.modifystate", stateBlock);
			auto stream = std::istringstream(newSupportedEntities);
			auto supportedEntities = config::LoadSupportedEntityVersions(stream);
			auto iter = supportedEntities.find(model::Entity_Type_ModifyState);
			if(iter == supportedEntities.end()) {
				supportedEntities.insert(std::make_pair(model::Entity_Type_ModifyState, std::unordered_set<VersionType>({1})));
			}
			else {
				if(iter->second.find(1) == iter->second.end()) {
					iter->second.insert(1);
				}
			}
			newSupportedEntities = config::SaveSupportedEntityVersionsToString(supportedEntities);

		}
		return state::NetworkConfigEntry(value.height(), newConfig, newSupportedEntities);
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
			bcEntry.offboardingReplicators().push_back(accountMigrationManager.GetKeyPair(offReplicator)->publicKey());
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
						value.greedObsolete(),
						value.disabledHeight(),
						value.version(),
						value.expirationTime(),
						value.activity(),
						value.feeInterest(),
						value.feeInterestDenominator(),
						value.bootKey(),
						value.blockchainVersion(),
						value.banPeriod());
	}
}}}
