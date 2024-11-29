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

#include "BlockGenerator.h"
#include "catapult/builders/BlockchainUpgradeBuilder.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/extensions/BlockExtensions.h"
#include "catapult/extensions/ConversionExtensions.h"
#include "catapult/extensions/IdGenerator.h"
#include "catapult/extensions/TransactionExtensions.h"
#include "catapult/model/Address.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/model/EntityHasher.h"
#include "StateBroker.h"
#include "catapult/utils/NetworkTime.h"
#include <inttypes.h>



namespace catapult { namespace tools { namespace nemgen {

	namespace {
		std::string GetChildName(const std::string& namespaceName) {
			return namespaceName.substr(namespaceName.rfind('.') + 1);
		}
	}
	model::UniqueEntityPtr<model::Block> CreateNemesisBlock(const NemesisConfiguration& config, const std::string& resourcesPath) {
		auto signer = crypto::KeyPair::FromString(config.NemesisSignerPrivateKey);
		NemesisTransactions transactions(signer, config);

		// - namespace creation
		for (const auto& rootPair : config.RootNamespaces) {
			// - root
			const auto& root = rootPair.second;
			const auto& rootName = config.NamespaceNames.at(root.id());
			auto duration = std::numeric_limits<BlockDuration::ValueType>::max() == root.lifetime().End.unwrap()
					? Eternal_Artifact_Duration
					: BlockDuration((root.lifetime().End - root.lifetime().Start).unwrap());
			transactions.addRegisterNamespace(rootName, duration);

			// - children
			std::map<size_t, std::vector<state::Namespace::Path>> paths;
			for (const auto& childPair : root.children()) {
				const auto& path = childPair.second.Path;
				paths[path.size()].push_back(path);
			}

			for (const auto& pair : paths) {
				for (const auto& path : pair.second) {
					const auto& child = state::Namespace(path);
					auto subName = GetChildName(config.NamespaceNames.at(child.id()));
					transactions.addRegisterNamespace(subName, child.parentId());
				}
			}
		}

		// - mosaic creation
		MosaicNonce nonce;
		std::map<std::string, UnresolvedMosaicId> nameToMosaicIdMap;
		for (const auto& mosaicPair : config.MosaicEntries) {
			const auto& mosaicEntry = mosaicPair.second;

			// - definition
			auto mosaicId = transactions.addMosaicDefinition(nonce, mosaicEntry.definition().properties());
			CATAPULT_LOG(debug) << "mapping " << mosaicPair.first << " to " << utils::HexFormat(mosaicId) << " (nonce " << nonce << ")";
			nonce = nonce + MosaicNonce(1);

			// - alias
			auto unresolvedMosaicId = transactions.addMosaicAlias(mosaicPair.first, mosaicId);
			nameToMosaicIdMap.emplace(mosaicPair.first, unresolvedMosaicId);

			// - supply
			transactions.addMosaicSupplyChange(unresolvedMosaicId, mosaicEntry.supply());
		}

		// - mosaic distribution
		for (const auto& addressMosaicSeedsPair : config.NemesisAddressToMosaicSeeds) {
			auto recipient = model::StringToAddress(addressMosaicSeedsPair.first);
			transactions.addTransfer(nameToMosaicIdMap, recipient, addressMosaicSeedsPair.second);
		}

		transactions.addConfig(resourcesPath);
		transactions.addUpgrade();

		for (const auto& key : config.HarvesterPrivateKeys) {
			transactions.addHarvester(key);
		}

		model::PreviousBlockContext context;
		auto pBlock = model::CreateBlock(context, config.NetworkIdentifier, signer.publicKey(), transactions.transactions());
		pBlock->Difficulty = Difficulty(NEMESIS_BLOCK_DIFFICULTY);
		pBlock->Type = model::Entity_Type_Nemesis_Block;
		pBlock->FeeInterest = 1;
		pBlock->FeeInterestDenominator = 1;
		pBlock->Timestamp = utils::NetworkTime();
		extensions::BlockExtensions(config.NemesisGenerationHash).signFullBlock(signer, *pBlock);
		return pBlock;
	}

	Hash256 UpdateNemesisBlock(
			const NemesisConfiguration& config,
			model::Block& block,
			NemesisExecutionHashesDescriptor& executionHashesDescriptor) {
		block.BlockReceiptsHash = executionHashesDescriptor.ReceiptsHash;
		block.StateHash = executionHashesDescriptor.StateHash;

		auto signer = crypto::KeyPair::FromString(config.NemesisSignerPrivateKey);
		if(!config.EnableSpool)
			extensions::BlockExtensions(config.NemesisGenerationHash).signFullBlock(signer, block);
		else
			model::SignBlockHeader(signer, block);
		return model::CalculateHash(block);
	}

	model::BlockElement CreateNemesisBlockElement(const NemesisConfiguration& config, const model::Block& block) {
		auto registry = CreateTransactionRegistry();
		auto generationHash = config.NemesisGenerationHash;
		return extensions::BlockExtensions(generationHash, registry).convertBlockToBlockElement(block, generationHash);
	}

	model::BlockElement ReconstructNemesisBlockElement(const NemesisConfiguration& config, const model::Block& block, const NemesisTransactions& nemesisTransactions) {
		model::BlockElement blockElement(block);
		blockElement.EntityHash = model::CalculateHash(block);
		blockElement.GenerationHash = config.NemesisGenerationHash;
		if(!config.EnableSpool)
		{
			auto transactionsView = nemesisTransactions.createView();
			for (const auto& transaction : transactionsView) {
				blockElement.Transactions.push_back(model::TransactionElement(*transaction.transaction));
				auto& transactionElement = blockElement.Transactions.back();
				transactionElement.EntityHash = transaction.entityHash;
				transactionElement.MerkleComponentHash = transaction.merkleHash;
			}
		}
		return blockElement;
	}
	model::UniqueEntityPtr<model::Block> CreateNemesisBlock(
			const model::PreviousBlockContext& context,
			model::NetworkIdentifier networkIdentifier,
			const Key& signerPublicKey,
			const uint64_t transactionPayloadSize,
			VersionType version) {
		auto headerSize = (version > 3) ? sizeof(model::BlockHeaderV4) : sizeof(model::BlockHeader);
		auto size = headerSize + transactionPayloadSize;
		auto pBlock = utils::MakeUniqueWithSize<model::Block>(size);
		std::memset(static_cast<void*>(pBlock.get()), 0, headerSize);
		pBlock->Size = static_cast<uint32_t>(size);
		pBlock->Version = MakeVersion(networkIdentifier, version);
		pBlock->setTransactionPayloadSize(transactionPayloadSize);

		pBlock->Signer = signerPublicKey;
		pBlock->Type = model::Entity_Type_Block;

		pBlock->Height = context.BlockHeight + Height(1);
		pBlock->Difficulty = Difficulty();
		pBlock->PreviousBlockHash = context.BlockHash;

		// we skip transaction copying as transactions are kept in temporary files at this point
		return pBlock;
	}

	model::UniqueEntityPtr<model::Block> ReconstructNemesisBlock(
			const NemesisConfiguration& config,
			NemesisTransactions& transactions,
			cache::CatapultCache& cache,
			const plugins::PluginManager& pluginManager,
			const std::shared_ptr<config::BlockchainConfigurationHolder> pConfig,
			StateBroker& broker) {

		auto signer = crypto::KeyPair::FromString(config.NemesisSignerPrivateKey);

		// Recreate nemesis transactions from existing state.
		broker.ProcessCaches(cache.height(), [&transactions](model::UniqueEntityPtr<model::Transaction>&& transaction){
			transactions.signAndAdd(std::move(transaction));
		});
		transactions.finalize();

		model::PreviousBlockContext context;
		auto pBlock = CreateNemesisBlock(context, config.NetworkIdentifier, signer.publicKey(), transactions.Size());
		pBlock->Difficulty = Difficulty(NEMESIS_BLOCK_DIFFICULTY);
		pBlock->Type = model::Entity_Type_Nemesis_Block;
		pBlock->FeeInterest = 1;
		pBlock->FeeInterestDenominator = 1;
		pBlock->Timestamp = utils::NetworkTime();

		if(config.EnableSpool) {
			// Sign transactionless block header.
			pBlock->BlockTransactionsHash = transactions.TxHash();
			model::SignBlockHeader(signer, *pBlock);
			return pBlock;
		}
		// Regular signing
		extensions::BlockExtensions(config.NemesisGenerationHash).signFullBlock(signer, *pBlock);
		return pBlock;
	}
}}}
