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
			for (const auto& entry : balances)
				StreamMosaic(mosaicsArray, entry.first, entry.second);

			mosaicsArray << bson_stream::close_array;

			auto snapshotsArray = builder << "snapshots" << bson_stream::open_array;
			for (const auto& snapshot : balances.snapshots())
				StreamSnapshot(snapshotsArray, snapshot.Amount, snapshot.BalanceHeight);

			snapshotsArray << bson_stream::close_array;
			return builder;
		}
	}

	bsoncxx::document::value ToDbModel(const state::AccountState& accountState) {
		// account metadata
		bson_stream::document builder;
		StreamAccountMetadata(builder);

		// account data
		builder << "account" << bson_stream::open_document
				<< "version" << static_cast<int32_t>(accountState.getVersion())
				<< "address" << ToBinary(accountState.Address)
				<< "addressHeight" << ToInt64(accountState.AddressHeight)
				<< "publicKey" << ToBinary(accountState.PublicKey)
				<< "publicKeyHeight" << ToInt64(accountState.PublicKeyHeight)
				<< "accountType" << utils::to_underlying_type(accountState.AccountType)
				<< "linkedAccountKey" << ToBinary(accountState.LinkedAccountKey);
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

		void ToAccountBalanceSnapshot(state::AccountBalances& accountBalances, const bsoncxx::document::view& mosaicDocument) {
			accountBalances.addSnapshot(
					model::BalanceSnapshot{
						GetValue64<Amount>(mosaicDocument["amount"]),
				        GetValue64<Height>(mosaicDocument["height"])
				    }
			);
		}
	}

	void ToAccountState(const bsoncxx::document::view& document, const AccountStateFactory& accountStateFactory) {
		auto accountDocument = document["account"];
		VersionType version = ToUint32(accountDocument["version"].get_int32());
		Address accountAddress;
		DbBinaryToModelArray(accountAddress, accountDocument["address"].get_binary());
		auto accountAddressHeight = GetValue64<Height>(accountDocument["addressHeight"]);

		auto& accountState = accountStateFactory(accountAddress, accountAddressHeight, version);
		DbBinaryToModelArray(accountState.PublicKey, accountDocument["publicKey"].get_binary());
		accountState.PublicKeyHeight = GetValue64<Height>(accountDocument["publicKeyHeight"]);

		accountState.AccountType = static_cast<state::AccountType>(ToUint8(accountDocument["accountType"].get_int32()));
		DbBinaryToModelArray(accountState.LinkedAccountKey, accountDocument["linkedAccountKey"].get_binary());

		auto dbMosaics = accountDocument["mosaics"].get_array().value;
		for (const auto& mosaicEntry : dbMosaics)
			ToAccountBalance(accountState.Balances, mosaicEntry.get_document().view());

		auto dbSnapshots = accountDocument["snapshots"].get_array().value;
		for (const auto& snapshotEntry : dbSnapshots)
			ToAccountBalanceSnapshot(accountState.Balances, snapshotEntry.get_document().view());
	}

	// endregion
}}}
