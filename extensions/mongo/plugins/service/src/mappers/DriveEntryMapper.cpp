/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DriveEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	namespace {
		void StreamPaymentInformation(bson_stream::document& builder, const std::vector<state::PaymentInformation>& payments) {
			auto array = builder << "payments" << bson_stream::open_array;
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
				StreamPaymentInformation(billingBuilder, description.Payments);
				array << billingBuilder;
			}

			array << bson_stream::close_array;
		}

        void StreamFileActions(bson_stream::document& builder, const std::vector<state::DriveAction>& actions) {
            auto array = builder << "actions" << bson_stream::open_array;
            for (const auto& action : actions) {
                array
                        << bson_stream::open_document
                        << "type" << static_cast<int8_t>(action.Type)
                        << "height" << ToInt64(action.ActionHeight)
                        << bson_stream::close_document;
            }

            array << bson_stream::close_array;
        }

		void StreamFiles(bson_stream::document& builder, const state::FilesMap& files) {
			auto array = builder << "files" << bson_stream::open_array;
			for (const auto& filePair : files) {
				bson_stream::document fileBuilder;
                fileBuilder
								<< "fileHash" << ToBinary(filePair.first)
								<< "deposit" << ToInt64(filePair.second.Deposit)
								<< "size" << static_cast<int64_t>(filePair.second.Size);
				StreamPaymentInformation(fileBuilder, filePair.second.Payments);
                StreamFileActions(fileBuilder, filePair.second.Actions);
				array << fileBuilder;
			}

			array << bson_stream::close_array;
		}

        void StreamFilesWithoutDeposit(bson_stream::document& builder, const std::map<Hash256, uint16_t>& filesWithoutDeposit) {
            auto array = builder << "filesWithoutDeposit" << bson_stream::open_array;
            for (const auto& file : filesWithoutDeposit) {
                array
                        << bson_stream::open_document
                        << "fileHash" << ToBinary(file.first)
                        << "count" << static_cast<int16_t>(file.second)
                        << bson_stream::close_document;
            }

            array << bson_stream::close_array;
        }

		void StreamReplicators(bson_stream::document& builder, const state::ReplicatorsMap & replicators) {
			auto array = builder << "replicators" << bson_stream::open_array;

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
							<< "end" << ToInt64(replicator.second->End)
							<< "deposit" << ToInt64(replicator.second->Deposit);
					StreamFilesWithoutDeposit(replicatorBuilder, replicator.second->FilesWithoutDeposit);
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
				<< "state" << static_cast<int8_t >(entry.state())
				<< "owner" << ToBinary(entry.owner())
				<< "rootHash" << ToBinary(entry.rootHash())
				<< "duration" << ToInt64(entry.duration())
				<< "billingPeriod" << ToInt64(entry.billingPeriod())
				<< "billingPrice" << ToInt64(entry.billingPrice())
				<< "size" << static_cast<int64_t>(entry.size())
				<< "replicas" << entry.replicas()
				<< "minReplicators" << static_cast<int8_t>(entry.minReplicators())
				<< "percentApprovers" << static_cast<int8_t>(entry.percentApprovers());

		StreamBillingHistory(builder, entry.billingHistory());
		StreamFiles(builder, entry.files());
		StreamReplicators(builder, entry.replicators());

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

        void ReadFileActions(std::vector<state::DriveAction>& actions, const bsoncxx::array::view& dbActionsMap) {
            for (const auto& dbAction : dbActionsMap) {
                auto doc = dbAction.get_document().view();

                state::DriveAction action;
                action.Type = static_cast<state::DriveActionType>(static_cast<int8_t>(doc["type"].get_int32()));
                action.ActionHeight = Height(doc["height"].get_int64());

                actions.emplace_back(action);
            }
        }

		void ReadFiles(state::FilesMap& files, const bsoncxx::array::view& dbFilesMap) {
			for (const auto& dbFile : dbFilesMap) {
				auto doc = dbFile.get_document().view();

                Hash256 fileHash;
                DbBinaryToModelArray(fileHash, doc["fileHash"].get_binary());

                state::FileInfo info;
                info.Deposit = Amount(doc["deposit"].get_int64());
                info.Size = doc["size"].get_int64();

				ReadPaymentInformation(info.Payments, doc["payments"].get_array().value);
                ReadFileActions(info.Actions, doc["actions"].get_array().value);

                files.emplace(fileHash, info);
			}
		}

        void ReadFilesWithoutDeposit(std::map<Hash256, uint16_t>& deposits, const bsoncxx::array::view& dbFilesWithoutDepositMap) {
            for (const auto& dbDeposit : dbFilesWithoutDepositMap) {
                auto doc = dbDeposit.get_document().view();

                Hash256 fileHash;
                DbBinaryToModelArray(fileHash, doc["fileHash"].get_binary());

                deposits.insert({ fileHash, static_cast<int16_t>(doc["count"].get_int32()) });
            }
        }

        void ReadReplicators(state::ReplicatorsMap& replicators, const bsoncxx::array::view& dbReplicatorsMap) {
            for (const auto& dbReplicator : dbReplicatorsMap) {
                auto doc = dbReplicator.get_document().view();

                Key replicator;
                DbBinaryToModelArray(replicator, doc["replicator"].get_binary());

                state::ReplicatorInfo info;
                info.Deposit = Amount(doc["deposit"].get_int64());
                info.Start = Height(doc["start"].get_int64());
                info.End = Height(doc["end"].get_int64());

                ReadFilesWithoutDeposit(info.FilesWithoutDeposit, doc["payments"].get_array().value);

                replicators.insert({ replicator, info });
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
        entry.setReplicas(static_cast<int8_t>(dbDriveEntry["replicas"].get_int32()));
        entry.setMinReplicators(static_cast<int8_t>(dbDriveEntry["minReplicators"].get_int32()));
        entry.setPercentApprovers(static_cast<int8_t>(dbDriveEntry["percentApprovers"].get_int32()));

		ReadBillingHistory(entry.billingHistory(), dbDriveEntry["billingHistory"].get_array().value);
		ReadFiles(entry.files(), dbDriveEntry["files"].get_array().value);
		ReadReplicators(entry.replicators(), dbDriveEntry["replicators"].get_array().value);

		return entry;
	}

	// endregion
}}}
