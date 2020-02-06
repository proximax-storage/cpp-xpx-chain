/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DriveEntryMapper.h"
#include "catapult/utils/Casting.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	namespace {
		void StreamPaymentInformation(bson_stream::document& builder, const std::string& name, const std::vector<state::PaymentInformation>& payments) {
			auto array = builder << name << bson_stream::open_array;
			for (const auto& payment : payments) {
				array
					<< bson_stream::open_document
					<< "receiver" << ToBinary(payment.Receiver)
					<< "amount" << ToInt64(payment.Amount)
					<< "height" << ToInt64(payment.Height)
					<< bson_stream::close_document;
			}

			array << bson_stream::close_array;
		}

		void StreamBillingHistory(bson_stream::document& builder, const std::vector<state::BillingPeriodDescription>& billingHistory) {
			auto array = builder << "billingHistory" << bson_stream::open_array;
			for (const auto& description : billingHistory) {
				bson_stream::document billingBuilder;
				billingBuilder
					<< "start" << ToInt64(description.Start)
					<< "end" << ToInt64(description.End);
				StreamPaymentInformation(billingBuilder, "payments", description.Payments);
				array << billingBuilder;
			}

			array << bson_stream::close_array;
		}

		void StreamFiles(bson_stream::document& builder, const state::FilesMap& files) {
			auto array = builder << "files" << bson_stream::open_array;
			for (const auto& filePair : files) {
				bson_stream::document fileBuilder;
				fileBuilder
					<< "fileHash" << ToBinary(filePair.first)
					<< "size" << static_cast<int64_t>(filePair.second.Size);
				array << fileBuilder;
			}

			array << bson_stream::close_array;
		}

		void StreamActiveFilesWithoutDeposit(bson_stream::document& builder, const std::set<Hash256>& activeFilesWithoutDeposit) {
			auto array = builder << "activeFilesWithoutDeposit" << bson_stream::open_array;
			for (const auto& file : activeFilesWithoutDeposit) {
				array << ToBinary(file);
			}

			array << bson_stream::close_array;
		}

		void StreamInactiveFilesWithoutDeposit(bson_stream::document& builder, const std::map<Hash256, std::vector<Height>>& inactiveFilesWithoutDeposit) {
			auto array = builder << "inactiveFilesWithoutDeposit" << bson_stream::open_array;
			for (const auto& filePair : inactiveFilesWithoutDeposit) {
				bson_stream::document replicatorBuilder;
				replicatorBuilder
					<< "fileHash" << ToBinary(filePair.first);

				auto heights = replicatorBuilder << "heights" << bson_stream::open_array;
				for (const auto& height : filePair.second)
					heights << ToInt64(height);

				heights << bson_stream::close_array;
				array << replicatorBuilder;
			}

			array << bson_stream::close_array;
		}

		template<typename T>
		void StreamReplicators(bson_stream::document& builder, const std::string& name, const T& replicators) {
			auto array = builder << name << bson_stream::open_array;

			std::map<Height, std::map<Key, const state::ReplicatorInfo*>> sorter;

			for (const auto& replicator : replicators) {
				sorter[replicator.second.Start][replicator.first] = &replicator.second;
			}

			for (const auto& replicatorHeight : sorter) {
				for (const auto& replicator : replicatorHeight.second) {
					bson_stream::document replicatorBuilder;
					replicatorBuilder
							<< "replicator" << ToBinary(replicator.first)
							<< "start" << ToInt64(replicator.second->Start)
							<< "end" << ToInt64(replicator.second->End);
					StreamActiveFilesWithoutDeposit(replicatorBuilder, replicator.second->ActiveFilesWithoutDeposit);
					StreamInactiveFilesWithoutDeposit(replicatorBuilder, replicator.second->InactiveFilesWithoutDeposit);
					array << replicatorBuilder;
				}
			}

			array << bson_stream::close_array;
		}
	}

	bsoncxx::document::value ToDbModel(const state::DriveEntry& entry, const Address& accountAddress) {
		bson_stream::document builder;
		auto doc = builder << "drive" << bson_stream::open_document
				<< "multisig" << ToBinary(entry.key())
				<< "multisigAddress" << ToBinary(accountAddress)
				<< "start" << ToInt64(entry.start())
				<< "end" << ToInt64(entry.end())
				<< "state" << utils::to_underlying_type(entry.state())
				<< "owner" << ToBinary(entry.owner())
				<< "rootHash" << ToBinary(entry.rootHash())
				<< "duration" << ToInt64(entry.duration())
				<< "billingPeriod" << ToInt64(entry.billingPeriod())
				<< "billingPrice" << ToInt64(entry.billingPrice())
				<< "size" << static_cast<int64_t>(entry.size())
				<< "occupiedSpace" << static_cast<int64_t>(entry.occupiedSpace())
				<< "replicas" << entry.replicas()
				<< "minReplicators" << static_cast<int16_t>(entry.minReplicators())
				<< "percentApprovers" << static_cast<int8_t>(entry.percentApprovers());

		StreamBillingHistory(builder, entry.billingHistory());
		StreamFiles(builder, entry.files());
		StreamReplicators(builder, "replicators", entry.replicators());
		StreamReplicators(builder, "removedReplicators", entry.removedReplicators());
		StreamPaymentInformation(builder, "uploadPayments", entry.uploadPayments());

		{
			auto array = builder << "coowners" << bson_stream::open_array;
			for (const auto& coowner : entry.coowners()) {
				array << ToBinary(coowner);
			}
			array << bson_stream::close_array;
		}

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	namespace {
		void ReadPaymentInformation(std::vector<state::PaymentInformation>& payments, const bsoncxx::array::view& dbPaymentsMap) {
			for (const auto& dbPayment : dbPaymentsMap) {
				auto doc = dbPayment.get_document().view();

				state::PaymentInformation payment;

				Key receiver;
				DbBinaryToModelArray(receiver, doc["receiver"].get_binary());
				payment.Receiver = receiver;
				payment.Height = Height(doc["height"].get_int64());
				payment.Amount = Amount(doc["amount"].get_int64());
				payments.emplace_back(payment);
			}
		}

		void ReadBillingHistory(std::vector<state::BillingPeriodDescription>& billingHistory, const bsoncxx::array::view& dbBillingHistoryMap) {
			for (const auto& dbDescriptionMap : dbBillingHistoryMap) {
				auto doc = dbDescriptionMap.get_document().view();

				state::BillingPeriodDescription description;
				description.Start = Height(doc["start"].get_int64());
				description.End = Height(doc["end"].get_int64());

				ReadPaymentInformation(description.Payments, doc["payments"].get_array().value);
				billingHistory.emplace_back(description);
			}
		}

		void ReadFiles(state::FilesMap& files, const bsoncxx::array::view& dbFilesMap) {
			for (const auto& dbFile : dbFilesMap) {
				auto doc = dbFile.get_document().view();

				Hash256 fileHash;
				DbBinaryToModelArray(fileHash, doc["fileHash"].get_binary());

				state::FileInfo info;
				info.Size = doc["size"].get_int64();

				files.emplace(fileHash, info);
			}
		}

		void ReadActiveFilesWithoutDeposit(std::set<Hash256>& deposits, const bsoncxx::array::view& dbFilesWithoutDepositMap) {
			for (const auto& dbDeposit : dbFilesWithoutDepositMap) {
				auto doc = dbDeposit.get_binary();

				Hash256 fileHash;
				DbBinaryToModelArray(fileHash, doc);

				deposits.insert(fileHash);
			}
		}

		void ReadInactiveFilesWithoutDeposit(std::map<Hash256, std::vector<Height>>& deposits, const bsoncxx::array::view& dbFilesWithoutDepositMap) {
			for (const auto& dbDeposit : dbFilesWithoutDepositMap) {
				auto doc = dbDeposit.get_document().view();

				Hash256 fileHash;
				DbBinaryToModelArray(fileHash, doc["fileHash"].get_binary());

				std::vector<Height> heights;
				for (const auto& dbHeight : doc["heights"].get_array().value)
					heights.push_back(Height(dbHeight.get_int64()));

				deposits.insert({ fileHash, heights });
			}
		}

		template<typename T>
		void insert(std::vector<T>& replicators, const T& p) {
			replicators.push_back(p);
		}

		template<typename T1, typename T2>
		void insert(std::map<T1, T2>& replicators, const std::pair<T1, T2>& p) {
			replicators.insert(p);
		}

		template<typename T>
		void ReadReplicators(T& replicators, const bsoncxx::array::view& dbReplicatorsMap) {
			for (const auto& dbReplicator : dbReplicatorsMap) {
				auto doc = dbReplicator.get_document().view();

				Key replicator;
				DbBinaryToModelArray(replicator, doc["replicator"].get_binary());

				state::ReplicatorInfo info;
				info.Start = Height(doc["start"].get_int64());
				info.End = Height(doc["end"].get_int64());

				ReadActiveFilesWithoutDeposit(info.ActiveFilesWithoutDeposit, doc["activeFilesWithoutDeposit"].get_array().value);
				ReadInactiveFilesWithoutDeposit(info.InactiveFilesWithoutDeposit, doc["inactiveFilesWithoutDeposit"].get_array().value);

				insert(replicators, { replicator, info });
			}
		}
	}

	state::DriveEntry ToDriveEntry(const bsoncxx::document::view& document) {
		auto dbDriveEntry = document["drive"];
		Key multisig;
		DbBinaryToModelArray(multisig, dbDriveEntry["multisig"].get_binary());
		state::DriveEntry entry(multisig);

		entry.setState(static_cast<state::DriveState>(static_cast<uint8_t>(dbDriveEntry["state"].get_int32())));

		Key owner;
		DbBinaryToModelArray(owner, dbDriveEntry["owner"].get_binary());
		entry.setOwner(owner);

		Hash256 rootHash;
		DbBinaryToModelArray(rootHash, dbDriveEntry["rootHash"].get_binary());
		entry.setRootHash(rootHash);

		entry.setStart(Height(dbDriveEntry["start"].get_int64()));
		entry.setEnd(Height(dbDriveEntry["end"].get_int64()));
		entry.setDuration(BlockDuration(dbDriveEntry["duration"].get_int64()));
		entry.setBillingPeriod(BlockDuration(dbDriveEntry["billingPeriod"].get_int64()));
		entry.setBillingPrice(Amount(dbDriveEntry["billingPrice"].get_int64()));
		entry.setSize(static_cast<uint64_t>(dbDriveEntry["size"].get_int64()));
		entry.setOccupiedSpace(static_cast<uint64_t>(dbDriveEntry["occupiedSpace"].get_int64()));
		entry.setReplicas(static_cast<uint16_t>(dbDriveEntry["replicas"].get_int32()));
		entry.setMinReplicators(static_cast<uint16_t>(dbDriveEntry["minReplicators"].get_int32()));
		entry.setPercentApprovers(static_cast<uint8_t>(dbDriveEntry["percentApprovers"].get_int32()));

		ReadBillingHistory(entry.billingHistory(), dbDriveEntry["billingHistory"].get_array().value);
		ReadFiles(entry.files(), dbDriveEntry["files"].get_array().value);
		ReadReplicators(entry.replicators(), dbDriveEntry["replicators"].get_array().value);
		ReadReplicators(entry.removedReplicators(), dbDriveEntry["removedReplicators"].get_array().value);
		ReadPaymentInformation(entry.uploadPayments(), dbDriveEntry["uploadPayments"].get_array().value);

		for (const auto& dbCoowner : dbDriveEntry["coowners"].get_array().value) {
			Key coowner;
			DbBinaryToModelArray(coowner, dbCoowner.get_binary());
			entry.coowners().insert(coowner);
		}

		return entry;
	}

	// endregion
}}}
