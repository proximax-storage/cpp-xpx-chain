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

#include "AccountStateMapper.h"
#include "MapperUtils.h"
#include "catapult/state/AccountState.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace mongo { namespace mappers {

	// region ToDbModel

	namespace {
		void StreamAccountMetadata(bson_stream::document& builder) {
			builder << "meta"
					<< bson_stream::open_document
					<< bson_stream::close_document;
		}

		auto& StreamAccountBalances(bson_stream::document& builder, const state::AccountBalances& balances) {
			auto mosaicsArray = builder << "mosaics" << bson_stream::open_array;
			for (const auto& entry : balances.balances())
				StreamMosaic(mosaicsArray, entry.first, entry.second);

			mosaicsArray << bson_stream::close_array;

			auto lockedMosaicsArray = builder << "lockedMosaics" << bson_stream::open_array;
			for (const auto& entry : balances.lockedBalances())
				StreamMosaic(lockedMosaicsArray, entry.first, entry.second);

			lockedMosaicsArray << bson_stream::close_array;

			auto snapshotsArray = builder << "snapshots" << bson_stream::open_array;
			for (const auto& snapshot : balances.snapshots())
				StreamSnapshot(snapshotsArray, snapshot.Amount, snapshot.LockedAmount, snapshot.BalanceHeight);

			snapshotsArray << bson_stream::close_array;
			return builder;
		}
	}

	void StreamPublicKey(
			bson_stream::document& builder,
			const std::string& name,
			state::AccountPublicKeys::KeyType mask,
			state::AccountPublicKeys::KeyType keyType,
			const state::AccountPublicKeys::PublicKeyAccessor<Key>& publicKeyAccessor) {
		if (!HasFlag(keyType, mask))
			return;

		builder
				<< name << bson_stream::open_document
				<< "publicKey" << ToBinary(publicKeyAccessor.get())
				<< bson_stream::close_document;
	}
	void StreamAccountPublicKeys(bson_stream::document& builder, const state::AccountPublicKeys& accountPublicKeys, uint32_t version) {
		builder << "supplementalPublicKeys" << bson_stream::open_document;

		auto mask = accountPublicKeys.mask();
		StreamPublicKey(builder, "linked", mask, state::AccountPublicKeys::KeyType::Linked, accountPublicKeys.linked());
		if(version > 1)
		{
			StreamPublicKey(builder, "node", mask, state::AccountPublicKeys::KeyType::Node, accountPublicKeys.node());
			StreamPublicKey(builder, "vrf", mask, state::AccountPublicKeys::KeyType::VRF, accountPublicKeys.vrf());
			StreamPublicKey(builder, "upgrade", mask, state::AccountPublicKeys::KeyType::Upgrade, accountPublicKeys.upgrade());
		}

		builder << bson_stream::close_document;
	}
	bsoncxx::document::value ToDbModel(const state::AccountState& accountState) {
		// account metadata
		bson_stream::document builder;
		StreamAccountMetadata(builder);

		// account data
		builder << "account" << bson_stream::open_document
				<< "version" << (int32_t)accountState.GetVersion()
				<< "address" << ToBinary(accountState.Address)
				<< "addressHeight" << ToInt64(accountState.AddressHeight)
				<< "publicKey" << ToBinary(accountState.PublicKey)
				<< "publicKeyHeight" << ToInt64(accountState.PublicKeyHeight)
				<< "accountType" << utils::to_underlying_type(accountState.AccountType);
		if(accountState.OldState) {
			builder << "upgradedFrom" << ToBinary(accountState.OldState->PublicKey);
		}
		StreamAccountPublicKeys(builder, accountState.SupplementalPublicKeys, accountState.GetVersion());
		StreamAccountBalances(builder, accountState.Balances);
		builder << bson_stream::close_document;
		return builder << bson_stream::finalize;
	}

	// endregion

	// region ToAccountState

	namespace {
		void ToAccountBalance(state::AccountBalances& accountBalances, const bsoncxx::document::view& mosaicDocument) {
			accountBalances.credit(GetValue64<MosaicId>(mosaicDocument["id"]), GetValue64<Amount>(mosaicDocument["amount"]));
		}
		void ToAccountLockedBalance(state::AccountBalances& accountBalances, const bsoncxx::document::view& mosaicDocument) {
			accountBalances.credit(GetValue64<MosaicId>(mosaicDocument["id"]), GetValue64<Amount>(mosaicDocument["amount"]));
			accountBalances.lock(GetValue64<MosaicId>(mosaicDocument["id"]), GetValue64<Amount>(mosaicDocument["amount"]));
		}

		void ToAccountBalanceSnapshot(state::AccountBalances& accountBalances, const bsoncxx::document::view& mosaicDocument) {
			accountBalances.addSnapshot(
					model::BalanceSnapshot{
						GetValue64<Amount>(mosaicDocument["amount"]),
						GetValue64<Amount>(mosaicDocument["lockedAmount"]),
				        GetValue64<Height>(mosaicDocument["height"])
				    }
			);
		}
		void ToSupplementalAccountKeys(state::AccountPublicKeys& keys, const bsoncxx::document::view& supplementalKeysDocument, uint32_t version)
		{
			Key tempVal;
			if(supplementalKeysDocument["linked"])
			{
				DbBinaryToModelArray(tempVal, supplementalKeysDocument["linked"]["publicKey"].get_binary());
				keys.linked().unset();
				keys.linked().set(tempVal);
			}
			if(version == 1) return;
			if(supplementalKeysDocument["node"])
			{
				DbBinaryToModelArray(tempVal, supplementalKeysDocument["node"]["publicKey"].get_binary());
				keys.node().unset();
				keys.node().set(tempVal);
			}
			if(supplementalKeysDocument["vrf"])
			{
				DbBinaryToModelArray(tempVal, supplementalKeysDocument["vrf"]["publicKey"].get_binary());
				keys.vrf().unset();
				keys.vrf().set(tempVal);
			}
		}
	}

	void ToAccountState(const bsoncxx::document::view& document, const AccountStateFactory& accountStateFactory) {
		auto accountDocument = document["account"];
		Address accountAddress;
		DbBinaryToModelArray(accountAddress, accountDocument["address"].get_binary());
		auto accountAddressHeight = GetValue64<Height>(accountDocument["addressHeight"]);

		auto accountState = accountStateFactory(accountAddress, accountAddressHeight, (uint32_t)accountDocument["version"].get_int32());
		DbBinaryToModelArray(accountState->PublicKey, accountDocument["publicKey"].get_binary());
		accountState->PublicKeyHeight = GetValue64<Height>(accountDocument["publicKeyHeight"]);
		accountState->AccountType = static_cast<state::AccountType>(ToUint8(accountDocument["accountType"].get_int32()));

		ToSupplementalAccountKeys(accountState->SupplementalPublicKeys, accountDocument["supplementalPublicKeys"].get_document().view(), accountState->GetVersion());
		auto dbMosaics = accountDocument["mosaics"].get_array().value;
		for (const auto& mosaicEntry : dbMosaics)
			ToAccountBalance(accountState->Balances, mosaicEntry.get_document().view());

		auto dbLockedMosaics = accountDocument["lockedMosaics"].get_array().value;
		for (const auto& mosaicEntry : dbLockedMosaics)
			ToAccountLockedBalance(accountState->Balances, mosaicEntry.get_document().view());

		auto dbSnapshots = accountDocument["snapshots"].get_array().value;
		for (const auto& snapshotEntry : dbSnapshots)
			ToAccountBalanceSnapshot(accountState->Balances, snapshotEntry.get_document().view());
	}

	// endregion
}}}
