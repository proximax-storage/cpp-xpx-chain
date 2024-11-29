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
	Address AccountMigrationManager::GetActiveAddress(const Address& sourceAddress) {
		return Get(sourceAddress);
	}

	Address AccountMigrationManager::GetActiveAddress(const Key& sourceKey) {
		return Get(model::PublicKeyToAddress(sourceKey, m_pHolder->Config().Immutable.NetworkIdentifier));
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

	void AccountMigrationManager::writeToFile(std::string path) {
		// Open the file for writing
		std::ofstream outFile(path);
		if (!outFile.is_open()) {
			// Handle error
			CATAPULT_THROW_RUNTIME_ERROR("Failed to open account dump file.");
		}

		// Iterate over the m_KeyPairMap
		for (auto it = m_KeyPairMap->begin(); it != m_KeyPairMap->end(); ++it) {
			const Address& address = it->first;
			const crypto::KeyPair& keyPair = it->second;

			std::string addressStr = model::AddressToString(address);
			std::string privateKeyStr = toHexString(keyPair.publicKey().data(), Key_Size);
			std::string publicKeyStr = toHexString(keyPair.privateKey().data(), Key_Size);

			// Write to file
			outFile << addressStr << " -> " << privateKeyStr << " -> " << publicKeyStr << std::endl;
		}

		outFile.close();
	}

}}
