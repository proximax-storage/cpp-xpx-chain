/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuperContractEntryMapper.h"
#include "catapult/utils/Casting.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	bsoncxx::document::value ToDbModel(const state::SuperContractEntry& entry, const Address& accountAddress) {
		bson_stream::document builder;
		auto doc = builder
			<< "supercontract" << bson_stream::open_document
			<< "multisig" << ToBinary(entry.key())
			<< "multisigAddress" << ToBinary(accountAddress)
			<< "state" << utils::to_underlying_type(entry.state())
			<< "owner" << ToBinary(entry.owner())
			<< "start" << ToInt64(entry.start())
			<< "end" << ToInt64(entry.end())
			<< "vmVersion" << ToInt64(entry.vmVersion())
			<< "mainDriveKey" << ToBinary(entry.mainDriveKey())
			<< "fileHash" << ToBinary(entry.fileHash())
			<< "executionCount" << entry.executionCount();

		return doc
			<< bson_stream::close_document
			<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::SuperContractEntry ToSuperContractEntry(const bsoncxx::document::view& document) {
		auto dbContractEntry = document["supercontract"];
		Key multisig;
		DbBinaryToModelArray(multisig, dbContractEntry["multisig"].get_binary());
		state::SuperContractEntry entry(multisig);

		Key owner;
		DbBinaryToModelArray(owner, dbContractEntry["owner"].get_binary());
		entry.setOwner(owner);

		Key mainDriveKey;
		DbBinaryToModelArray(mainDriveKey, dbContractEntry["mainDriveKey"].get_binary());
		entry.setMainDriveKey(mainDriveKey);

		Hash256 fileHash;
		DbBinaryToModelArray(fileHash, dbContractEntry["fileHash"].get_binary());
		entry.setFileHash(fileHash);

		entry.setState(static_cast<state::SuperContractState>(static_cast<uint8_t>(dbContractEntry["state"].get_int32())));
		entry.setStart(Height(dbContractEntry["start"].get_int64()));
		entry.setEnd(Height(dbContractEntry["end"].get_int64()));
		entry.setVmVersion(VmVersion(dbContractEntry["vmVersion"].get_int64()));
		entry.setExecutionCount(static_cast<uint16_t>(dbContractEntry["executionCount"].get_int32()));

		return entry;
	}

	// endregion
}}}
