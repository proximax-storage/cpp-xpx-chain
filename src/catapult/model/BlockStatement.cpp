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

#include "BlockStatement.h"
#include "catapult/crypto/MerkleHashBuilder.h"

namespace catapult { namespace model {

	namespace {
		template<typename TStatementMap>
		void AddAll(crypto::MerkleHashBuilder& builder, const TStatementMap& statementMap) {
			for (const auto& pair : statementMap)
				builder.update(pair.second.hash());
		}

		template<typename TOutput>
		void CalculateMerkleHash(const BlockStatement& statement, TOutput& output) {
			crypto::MerkleHashBuilder builder(CountTotalStatements(statement));

			AddAll(builder, statement.TransactionStatements);
			AddAll(builder, statement.AddressResolutionStatements);
			AddAll(builder, statement.MosaicResolutionStatements);
			AddAll(builder, statement.PublicKeyStatements);

			builder.final(output);
		}

		template<typename TStatement>
		void CopyStatements(
				std::map<ReceiptSource, TStatement>& destination,
				const std::map<ReceiptSource, TStatement>& source,
				uint32_t maxSourcePrimaryId) {
			for (const auto& pair : source) {
				if (pair.first.PrimaryId > maxSourcePrimaryId)
					continue;

				TStatement statement(pair.first);
				for (auto i = 0u; i < pair.second.size(); ++i)
					statement.addReceipt(pair.second.receiptAt(i));

				destination.emplace(pair.first, std::move(statement));
			}
		}

		template<typename TKey, typename TStatement>
		void CopyResolutionStatements(
				std::map<TKey, TStatement>& destination,
				const std::map<TKey, TStatement>& source,
				uint32_t maxSourcePrimaryId) {
			for (const auto& pair : source) {
				TStatement statement(pair.first);
				for (auto i = 0u; i < pair.second.size(); ++i) {
					const auto& entry = pair.second.entryAt(i);
					if (entry.Source.PrimaryId > maxSourcePrimaryId)
						continue;

					statement.addResolution(entry.ResolvedValue, entry.Source);
				}

				if (0 == statement.size())
					continue;

				destination.emplace(pair.first, std::move(statement));
			}
		}
	}

	Hash256 CalculateMerkleHash(const BlockStatement& statement) {
		Hash256 merkleHash;
		CalculateMerkleHash(statement, merkleHash);
		return merkleHash;
	}

	std::vector<Hash256> CalculateMerkleTree(const BlockStatement& statement) {
		std::vector<Hash256> merkleTree;
		CalculateMerkleHash(statement, merkleTree);
		return merkleTree;
	}

	size_t CountTotalStatements(const BlockStatement& statement) {
		return statement.TransactionStatements.size()
				+ statement.AddressResolutionStatements.size()
				+ statement.MosaicResolutionStatements.size()
				+ statement.PublicKeyStatements.size();
	}

	void DeepCopyTo(BlockStatement& destination, const BlockStatement& source) {
		DeepCopyTo(destination, source, std::numeric_limits<uint32_t>::max());
	}

	void DeepCopyTo(BlockStatement& destination, const BlockStatement& source, uint32_t maxSourcePrimaryId) {
		CopyStatements(destination.TransactionStatements, source.TransactionStatements, maxSourcePrimaryId);
		CopyResolutionStatements(destination.AddressResolutionStatements, source.AddressResolutionStatements, maxSourcePrimaryId);
		CopyResolutionStatements(destination.MosaicResolutionStatements, source.MosaicResolutionStatements, maxSourcePrimaryId);
		CopyStatements(destination.PublicKeyStatements, source.PublicKeyStatements, maxSourcePrimaryId);
	}
}}
