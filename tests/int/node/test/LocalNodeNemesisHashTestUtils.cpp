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

#include "LocalNodeNemesisHashTestUtils.h"
#include "sdk/src/extensions/BlockExtensions.h"
#include "plugins/txes/mosaic/src/model/MosaicEntityType.h"
#include "catapult/io/FileBlockStorage.h"
#include "catapult/io/RawFile.h"
#include "catapult/model/BlockStatementBuilder.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/model/EntityHasher.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/mocks/MockMemoryBlockStorage.h"
#include "tests/test/nodeps/MijinConstants.h"
#include "tests/test/nodeps/Nemesis.h"
#include "tests/test/nodeps/TestConstants.h"
#include "plugins/txes/lock_fund/src/model/LockFundTotalStakedReceipt.h"
#include "plugins/txes/lock_fund/src/model/LockFundReceiptType.h"
#include "catapult/extensions/NemesisBlockLoader.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "plugins/txes/transfer/src/config/TransferConfiguration.h"
#include "plugins/txes/namespace/src/config/NamespaceConfiguration.h"
#include "plugins/txes/lock_fund/src/config/LockFundConfiguration.h"
#include "plugins/txes/upgrade/src/config/BlockchainUpgradeConfiguration.h"
#include "plugins/txes/mosaic/src/config/MosaicConfiguration.h"
#include "plugins/txes/config/src/config/NetworkConfigConfiguration.h"
#include "plugins/services/globalstore/src/config/GlobalStoreConfiguration.h"
#include "plugins/txes/service/src/config/ServiceConfiguration.h"
#include "plugins/txes/exchange/src/config/ExchangeConfiguration.h"
#include "plugins/txes/supercontract/src/config/SuperContractConfiguration.h"
#include "plugins/txes/operation/src/config/OperationConfiguration.h"
#include "plugins/txes/metadata_v2/src/config/MetadataConfiguration.h"
#include "plugins/txes/restriction_account/src/config/AccountRestrictionConfiguration.h"

namespace catapult { namespace test {

	namespace {
		template<typename TModify>
		void ModifyNemesis(const std::string& destination, TModify modify) {
			// load from file storage to allow successive modifications
			io::FileBlockStorage storage(destination);
			auto pNemesisBlockElement = storage.loadBlockElement(Height(1));

			// modify nemesis block and resign it
			auto& nemesisBlock = const_cast<model::Block&>(pNemesisBlockElement->Block);
			auto bundleConfig = extensions::NemesisBlockLoader::ReadNetworkConfiguration(pNemesisBlockElement, config::ImmutableConfiguration::Uninitialized());
			modify(nemesisBlock, *pNemesisBlockElement, bundleConfig);
			extensions::BlockExtensions(GetNemesisGenerationHash()).signFullBlock(
					crypto::KeyPair::FromString(Mijin_Test_Nemesis_Private_Key, std::get<0>(bundleConfig).AccountVersion),
					nemesisBlock);

			// overwrite the nemesis file in destination
			// (only the block and entity hash need to be rewritten; this works because block size does not change)
			io::RawFile nemesisFile(destination + "/00000/00001.dat", io::OpenMode::Read_Append);
			nemesisFile.write({ reinterpret_cast<const uint8_t*>(&nemesisBlock), nemesisBlock.Size });
			nemesisFile.write(model::CalculateHash(nemesisBlock));
		}
	}

	namespace {
		std::vector<uint32_t> GetMosaicSupplyChangePrimarySourceIds(const model::Block& block) {
			uint32_t transactionIndex = 0;
			std::vector<uint32_t> primarySourceIds;
			for (const auto& transaction : block.Transactions()) {
				if (model::Entity_Type_Mosaic_Supply_Change == transaction.Type)
					primarySourceIds.push_back(transactionIndex + 1); // transaction sources are 1-based

				++transactionIndex;
			}

			if (5u != primarySourceIds.size())
				CATAPULT_THROW_INVALID_ARGUMENT_1("nemesis block has unexpected number of mosaic supply changes", primarySourceIds.size());

			return primarySourceIds;
		}
	}

	void SetNemesisReceiptsHash(const std::string& destination, const config::BlockchainConfiguration& config) {
		// calculate the receipts hash (default nemesis block has zeroed receipts hash)
		ModifyNemesis(destination,  [](auto& nemesisBlock, const auto&, const auto&) {
			model::BlockStatementBuilder blockStatementBuilder;

			// 1. add harvest fee receipt
			nemesisBlock.FeeInterest = 1;
			nemesisBlock.FeeInterestDenominator = 1;
			auto totalFee = model::CalculateBlockTransactionsInfo(nemesisBlock).TotalFee;

			auto feeMosaicId = Default_Currency_Mosaic_Id;
			model::BalanceChangeReceipt receipt(model::Receipt_Type_Harvest_Fee, nemesisBlock.Signer, feeMosaicId, totalFee);
			blockStatementBuilder.addTransactionReceipt(receipt);

			model::SignerBalanceReceipt signerBalanceReceipt(model::Receipt_Type_Block_Signer_Importance, Amount(0), Amount(0));
			blockStatementBuilder.addBlockchainStateReceipt(signerBalanceReceipt);

			model::TotalStakedReceipt totalStakedReceipt(model::Receipt_Type_Total_Staked, Amount(0));
			blockStatementBuilder.addBlockchainStateReceipt(totalStakedReceipt);

			model::GlobalStateChangeReceipt stateChangeReceipt(model::Receipt_Type_Blockchain_State_Tracking, MakeFlags(model::StateChangeFlags::Blockchain_Init));
			blockStatementBuilder.addBlockchainStateReceipt(stateChangeReceipt);

			// 2. add mosaic aliases (supply tx first uses alias, block mosaic order is deterministic)
			auto aliasFirstUsedPrimarySourceIds = GetMosaicSupplyChangePrimarySourceIds(nemesisBlock);
			blockStatementBuilder.setSource({ aliasFirstUsedPrimarySourceIds[0], 0 });
			blockStatementBuilder.addResolution(UnresolvedMosaicId(0x85BBEA6CC462B244), feeMosaicId);

			blockStatementBuilder.setSource({ aliasFirstUsedPrimarySourceIds[1], 0 });
			blockStatementBuilder.addResolution(UnresolvedMosaicId(0x87BEC7AE08A01FD1), Default_Storage_Mosaic_Id);

			blockStatementBuilder.setSource({ aliasFirstUsedPrimarySourceIds[2], 0 });
			blockStatementBuilder.addResolution(UnresolvedMosaicId(0xBF3108FFEDF8ADDB), Default_Streamin_Mosaic_Id);

			blockStatementBuilder.setSource({ aliasFirstUsedPrimarySourceIds[3], 0 });
			blockStatementBuilder.addResolution(UnresolvedMosaicId(0x9A547F75CBD687F4), Default_SuperContract_Mosaic_Id);

			blockStatementBuilder.setSource({ aliasFirstUsedPrimarySourceIds[4], 0 });
			blockStatementBuilder.addResolution(UnresolvedMosaicId(0x9D7ABFFA390FBDE7), Default_Review_Mosaic_Id);

			// 3. calculate the block receipts hash
			auto pStatement = blockStatementBuilder.build();
			nemesisBlock.BlockReceiptsHash = model::CalculateMerkleHash(*pStatement);
		});
	}
		void SetNemesisStateHash(const std::string& destination, const config::BlockchainConfiguration& config) {
		// calculate the state hash (default nemesis block has zeroed state hash)
		ModifyNemesis(destination, [&config](auto& nemesisBlock, const auto& nemesisBlockElement, const auto& configPair) {
			MutableBlockchainConfiguration mConfig;
			mConfig.Immutable = config.Immutable;
			mConfig.Node = config.Node;
			mConfig.User = config.User;
			mConfig.Extensions = config.Extensions;
			mConfig.Logging = config.Logging;
			mConfig.Network = std::get<0>(configPair);
			mConfig.SupportedEntityVersions = std::get<1>(configPair);

			// Preload plugin configuraitons for nemesis required plugins

			mConfig.Network.template InitPluginConfiguration<config::TransferConfiguration>();
			mConfig.Network.template InitPluginConfiguration<config::MosaicConfiguration>();
			mConfig.Network.template InitPluginConfiguration<config::NamespaceConfiguration>();
			mConfig.Network.template InitPluginConfiguration<config::BlockchainUpgradeConfiguration>();
			mConfig.Network.template InitPluginConfiguration<config::LockFundConfiguration>();
			mConfig.Network.template InitPluginConfiguration<config::GlobalStoreConfiguration>();
			mConfig.Network.template InitPluginConfiguration<config::AccountRestrictionConfiguration>();
			mConfig.Network.template InitPluginConfiguration<config::NetworkConfigConfiguration>();

			nemesisBlock.StateHash = CalculateNemesisStateHash(nemesisBlockElement, mConfig.ToConst());
		});
	}
}}
