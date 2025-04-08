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

#include "AccountMigrationManager.h"
#include "catapult/io/RawFile.h"
#include <fstream>

namespace catapult { namespace utils {

	namespace {
		std::string toHexString(const uint8_t* data, size_t length) {
			std::ostringstream oss;
			oss << std::hex << std::setfill('0');
			for (size_t i = 0; i < length; ++i)
				oss << std::setw(2) << static_cast<unsigned int>(data[i]);
			return oss.str();
		}
	}

	AccountMigrationManager::AccountMigrationManager(const std::string& nemesisPrivateKey, std::string path, std::shared_ptr<config::BlockchainConfigurationHolder> pHolder) : m_file(boost::interprocess::open_or_create, path.c_str(), 1000*1024*1024)
		, m_keyAllocator(m_file.get_segment_manager())
		, m_HashAllocator(m_file.get_segment_manager())
		, m_KeyPairAllocator(m_file.get_segment_manager())
		, m_vectorAllocator(m_file.get_segment_manager())
		, m_pHolder(pHolder)
		, m_NemesisPrivateKey(nemesisPrivateKey)
		, m_NemesisSet(false)
		, m_Generator()
	{
		m_HashMap = m_file.find_or_construct<PagedHashMap>("PagedHashMap")(std::less<Address>(), m_HashAllocator);
		m_keyMap = m_file.find_or_construct<PagedKeyMap>("PagedKeyMap")(std::less<Address>(), m_keyAllocator);
		m_KeyPairMap = m_file.find_or_construct<PagedKeyPairMap>("PagedKeyPairMap")(std::less<Address>(), m_KeyPairAllocator);
		m_metadataMap = m_file.find_or_construct<MetadataMap>("MetadataMap")(std::less<Address>(), m_vectorAllocator);
		m_HashMap->clear();
		m_keyMap->clear();
		m_KeyPairMap->clear();
		m_metadataMap->clear();
		m_path = path;
		auto keyPair = crypto::KeyPair::FromString(nemesisPrivateKey);
		auto address = model::PublicKeyToAddress(keyPair.publicKey(), m_pHolder->Config().Immutable.NetworkIdentifier);
		m_KeyPairMap->insert(std::make_pair(address, std::move(keyPair)));
		m_metadataMap->insert(std::make_pair(address, BipVector(m_file.get_segment_manager())));
		PushMetadata(address, "Nemesis Account");
	}

	Address AccountMigrationManager::GetActiveAddress(const Address& sourceAddress) {
		return Get(sourceAddress);
	}

	Address AccountMigrationManager::GetActiveAddress(const Key& sourceKey) {
		return Get(model::PublicKeyToAddress(sourceKey, m_pHolder->Config().Immutable.NetworkIdentifier));
	}

	void AccountMigrationManager::PushMetadata(const Address& address, std::string metadata)
	{
		auto iter = m_metadataMap->find(address);
		if(iter == m_metadataMap->end()) {
			CATAPULT_THROW_RUNTIME_ERROR("Adding metadata to non existing account");
		}
		iter->second.push_back(BipString(metadata, m_file.get_segment_manager()));

	}
	void AccountMigrationManager::PopMetadata(const Address& address) {
		auto iter = m_metadataMap->find(address);
		if(iter == m_metadataMap->end()) {
			CATAPULT_THROW_RUNTIME_ERROR("Modifying metadata from non existing account");
		}
		iter->second.pop_back();
	}

	boost::interprocess::offset_ptr<crypto::KeyPair> AccountMigrationManager::GetKeyPair(const Address& sourceAddress) {
		return &m_KeyPairMap->find(GetActiveAddress(sourceAddress))->second;
	}
	boost::interprocess::offset_ptr<crypto::KeyPair> AccountMigrationManager::GetKeyPair(const Key& sourceKey) {
		return &m_KeyPairMap->find(GetActiveAddress(sourceKey))->second;
	}

	Address AccountMigrationManager::Get(const Address& account) {
		auto iter = m_keyMap->find(account);
		if(iter == m_keyMap->end()) {
			auto keyPair = crypto::KeyPair::FromPrivate(crypto::PrivateKey::Generate(m_Generator));
			auto address = model::PublicKeyToAddress(keyPair.publicKey(), m_pHolder->Config().Immutable.NetworkIdentifier);
			m_keyMap->insert(std::make_pair(account, address));
			m_keyMap->insert(std::make_pair(address, address));
			m_KeyPairMap->insert(std::make_pair(address, std::move(keyPair)));
			m_metadataMap->insert(std::make_pair(address, BipVector(m_file.get_segment_manager())));
			Hash256 result;
			crypto::Sha3_256_Builder sha3;
			uint8_t empty = 0;
			sha3.update({
					{ reinterpret_cast<const uint8_t*>(&empty), sizeof(uint8_t) },
					address
			});
			sha3.final(result);
			m_HashMap->insert(std::make_pair(address, result));
			return address;
		}
		return iter->second;
	}

	void AccountMigrationManager::AssociateNemesis(const Key& publicKey) {
		AssociateNemesis(model::PublicKeyToAddress(publicKey, m_pHolder->Config().Immutable.NetworkIdentifier));
	}

	void AccountMigrationManager::AssociateNemesis(const Address& oldAddress) {
		if(m_NemesisSet)
			return;
		auto nemesisKeyPair = crypto::KeyPair::FromString(m_NemesisPrivateKey);
		auto address = model::PublicKeyToAddress(nemesisKeyPair.publicKey(), m_pHolder->Config().Immutable.NetworkIdentifier);
		auto nemesisIter = m_keyMap->find(address);
		if(nemesisIter != m_keyMap->end())
			CATAPULT_THROW_RUNTIME_ERROR("Attempt to associate more than one nemesis account in account manager.");
		m_keyMap->insert(std::make_pair(oldAddress, address));
		m_keyMap->insert(std::make_pair(address, address));
		m_NemesisSet = true;
	}

	void AccountMigrationManager::writeToFile(std::string path) {
		// Open the file for writing
		std::ofstream outFile(path);
		if (!outFile.is_open()) {
			// Handle error
			CATAPULT_THROW_RUNTIME_ERROR("Failed to open account dump file.");
		}

		// Iterate over the m_KeyPairMap
		for (auto it = m_keyMap->begin(); it != m_keyMap->end(); ++it) {
			if(it->first == it->second)
				continue;
			const Address& address = it->first;
			const Address& newAddress = it->second;
			const crypto::KeyPair& keyPair = m_KeyPairMap->find(newAddress)->second;
			auto metadata = m_metadataMap->find(newAddress)->second;
			std::string addressStr = model::AddressToString(address);
			std::string newAddressStr = model::AddressToString(newAddress);
			std::string privateKeyStr = toHexString(keyPair.publicKey().data(), Key_Size);
			std::string publicKeyStr = toHexString(keyPair.privateKey().data(), Key_Size);

			// Write to file
			outFile << addressStr << " : " << std::endl;
			outFile << newAddressStr << " -> " << privateKeyStr << " -> " << publicKeyStr << std::endl;

			for(auto metaString : metadata) {
				outFile << metaString << std::endl;
			}
			outFile << "----" << std::endl;
		}

		outFile.close();
	}

	AccountMigrationManager::~AccountMigrationManager() {
		if(boost::filesystem::exists(m_path)) {
			boost::filesystem::remove(m_path);
		}
	}

}}
