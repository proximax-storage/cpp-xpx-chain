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

#pragma once
#include "catapult/model/Block.h"
#include "catapult/model/Elements.h"
#include "catapult/plugins/PluginManager.h"
#include "catapult/model/BlockUtils.h"
#include "NemesisConfiguration.h"
#include "TransactionRegistryFactory.h"
#include "catapult/crypto/MerkleHashBuilder.h"
#include "catapult/io/BufferedFileStream.h"
#include <memory>

namespace catapult {
	namespace tools {
		namespace nemgen {
			struct NemesisConfiguration;
			struct NemesisExecutionHashesDescriptor;
		}
	}
}



namespace catapult { namespace tools { namespace nemgen {

	struct TransactionHashContainer {
		std::shared_ptr<model::Transaction> transaction;
		Hash256 entityHash;
		Hash256 merkleHash;
	};
	class NemesisTransactions {
	public:
		NemesisTransactions(
				const crypto::KeyPair& signer,
				const NemesisConfiguration& config,
				const model::TransactionRegistry& registry);


	public:
		struct NemesisTransactionsView {

			// Custom iterator
			class Iterator {
			public:
				using iterator_category = std::input_iterator_tag;
				using value_type = TransactionHashContainer;
				using difference_type = std::ptrdiff_t;

				explicit Iterator(io::RawFile& fileStream, bool& iteratorInUse, bool end = false)
					: m_rSpoolFile(fileStream), m_rIteratorInUse(iteratorInUse), m_End(end) {
					if (!m_End) {
						loadNextTransaction();
					}
				}

				// Destructor clears the flag when iterator is destroyed
				~Iterator() {
					if (!m_End) {
						m_rIteratorInUse = false;
					}
				}

				// Dereference operator
				value_type operator*() const {
					return m_Transaction;
				}

				// Increment operator
				Iterator& operator++() {
					loadNextTransaction();
					return *this;
				}

				bool operator==(const Iterator& other) const {
					return m_End == other.m_End;
				}

				bool operator!=(const Iterator& other) const {
					return !(*this == other);
				}

			private:

				void loadNextTransaction() {
					if (m_End) {
						return;
					}

					if (m_rSpoolFile.position() == m_rSpoolFile.size()) {
						m_End = true;
						m_rIteratorInUse = false; // Mark iterator as no longer in use
						return;
					}

					std::vector<uint8_t> vsize(sizeof(uint32_t));
					m_rSpoolFile.read(vsize);
					auto size = *reinterpret_cast<uint32_t*>(vsize.data());
					std::vector<uint8_t> tx(size-sizeof(uint32_t));
					m_rSpoolFile.read(tx);

					m_Transaction.transaction = utils::MakeSharedWithSize<model::Transaction>(size);
					memcpy(reinterpret_cast<uint8_t*>(m_Transaction.transaction.get()), vsize.data(), vsize.size());
					memcpy(reinterpret_cast<uint8_t*>(m_Transaction.transaction.get())+vsize.size(), tx.data(), tx.size());

					std::vector<uint8_t> hash(Hash256_Size);

					m_rSpoolFile.read(hash);
					memcpy(reinterpret_cast<uint8_t*>(&m_Transaction.entityHash), hash.data(), Hash256_Size);

					m_rSpoolFile.read(hash);
					memcpy(reinterpret_cast<uint8_t*>(&m_Transaction.merkleHash), hash.data(), Hash256_Size);
				}

			private:
				io::RawFile& m_rSpoolFile;
				TransactionHashContainer m_Transaction;
				bool m_End;
				bool& m_rIteratorInUse; // Reference to the shared "in-use" flag
			};

			explicit NemesisTransactionsView(io::RawFile&& spoolFile)
				: m_spoolFile(std::move(spoolFile)), m_IteratorInUse(false) {}

			Iterator begin() {
				if (m_IteratorInUse) {
					throw std::runtime_error("Iterator already in use for this view.");
				}
				m_IteratorInUse = true;
				m_spoolFile.seek(0);
				return Iterator(m_spoolFile, m_IteratorInUse);
			}

			Iterator end() {
				return Iterator(m_spoolFile, m_IteratorInUse, true);
			}

			io::RawFile m_spoolFile;
			bool m_IteratorInUse; // Flag to track if an iterator is active
		};
	public:

		void addRegisterNamespace(const std::string& namespaceName, BlockDuration duration);

		void addRegisterNamespace(const std::string& namespaceName, NamespaceId parentId);

		MosaicId addMosaicDefinition(MosaicNonce nonce, const model::MosaicProperties& properties);

		UnresolvedMosaicId addMosaicAlias(const std::string& mosaicName, MosaicId mosaicId);

		void addMosaicSupplyChange(UnresolvedMosaicId mosaicId, Amount delta);

		void addTransfer(
				const std::map<std::string, UnresolvedMosaicId>& mosaicNameToMosaicIdMap,
				const Address& recipientAddress,
				const std::vector<MosaicSeed>& seeds);

		void addConfig(const std::string& resourcesPath);

		void addUpgrade();

		void addHarvester(const std::string& harvesterPrivateKey);

	public:
		const std::vector<TransactionHashContainer>& transactions() const;

		void signAndAdd(model::UniqueEntityPtr<model::Transaction>&& pTransaction);

		void finalize();
	private:


		void signAndAdd(model::UniqueEntityPtr<model::Transaction>&& pTransaction, const crypto::KeyPair& signer);

		void trySpool();

		void spool();

	public:

		uint32_t Size() const;

		uint32_t Total() const;

		Hash256 TxHash() const;

		NemesisTransactionsView createView() const;

	private:
		crypto::MerkleHashBuilder m_builder;
		Hash256 m_hash;
		const crypto::KeyPair& m_signer;
		std::vector<TransactionHashContainer> m_transactions;
		uint32_t m_BufferSize;
		uint32_t m_TotalTransactions;
		std::unique_ptr<io::BufferedOutputFileStream> m_spoolFile;
		uint32_t m_totalSize;
		const model::TransactionRegistry& m_Registry;
		const NemesisConfiguration* m_Config;
	};

}}}
