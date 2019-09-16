/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ServiceMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/service/src/model/ServiceTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		void StreamMosaics(bson_stream::document& builder, const model::UnresolvedMosaic* pMosaic, size_t numMosaics) {
			auto mosaicsArray = builder << "mosaics" << bson_stream::open_array;
			for (auto i = 0u; i < numMosaics; ++i) {
				StreamMosaic(mosaicsArray, pMosaic->MosaicId, pMosaic->Amount);
				++pMosaic;
			}

			mosaicsArray << bson_stream::close_array;
		}

		void StreamFile(bson_stream::document& builder, const model::DriveFile& file) {
			builder
					<< "fileData" << bson_stream::open_document
					<< "hash" << ToBinary(file.Hash)
					<< "parentHash" << ToBinary(file.ParentHash)
					<< "name" << ToBinary(file.NamePtr(), file.NameSize)
					<< bson_stream::close_document;
		}

		void StreamAction(bson_stream::document& builder, model::DriveActionType type, const uint8_t* pAction) {
			builder
					<< "action" << bson_stream::open_document
					<< "actionType" << utils::to_underlying_type(type);

			switch (type) {
				case model::DriveActionType::DriveDeposit :
				case model::DriveActionType::DriveDepositReturn :
				case model::DriveActionType::DrivePayment :
				case model::DriveActionType::DriveVerification :
					break;

				case model::DriveActionType::PrepareDrive : {
					auto pActionPrepareDrive = reinterpret_cast<const model::ActionPrepareDrive*>(pAction);
					builder
							<< "duration" << ToInt64(pActionPrepareDrive->Duration)
							<< "size" << static_cast<int64_t>(pActionPrepareDrive->Size)
							<< "replicas" << pActionPrepareDrive->Replicas;
					break;
				}

				case model::DriveActionType::DriveProlongation : {
					auto pActionDriveProlongation = reinterpret_cast<const model::ActionDriveProlongation*>(pAction);
					builder
							<< "duration" << ToInt64(pActionDriveProlongation->Duration);
					break;
				}

				case model::DriveActionType::FileDeposit : {
					auto pActionFileDeposit = reinterpret_cast<const model::ActionFileDeposit*>(pAction);
					builder
							<< "fileHash" << ToBinary(pActionFileDeposit->FileHash);
					break;
				}

				case model::DriveActionType::FileDepositReturn : {
					auto pActionFileDepositReturn = reinterpret_cast<const model::ActionFileDepositReturn*>(pAction);
					builder
							<< "fileHash" << ToBinary(pActionFileDepositReturn->FileHash);
					break;
				}

				case model::DriveActionType::FilePayment : {
					auto pActionFileDeposit = reinterpret_cast<const model::ActionFileDeposit*>(pAction);
					builder
							<< "fileHash" << ToBinary(pActionFileDeposit->FileHash);
					break;
				}

				case model::DriveActionType::CreateDirectory : {
					auto pActionCreateDirectory = reinterpret_cast<const model::ActionCreateDirectory*>(pAction);
					builder << "directory";
					StreamFile(builder, pActionCreateDirectory->Directory);
					break;
				}

				case model::DriveActionType::RemoveDirectory : {
					auto pActionRemoveDirectory = reinterpret_cast<const model::ActionRemoveDirectory*>(pAction);
					builder << "directory";
					StreamFile(builder, pActionRemoveDirectory->Directory);
					break;
				}

				case model::DriveActionType::UploadFile : {
					auto pActionUploadFile = reinterpret_cast<const model::ActionUploadFile*>(pAction);
					builder << "file";
					StreamFile(builder, pActionUploadFile->File);
					break;
				}

				case model::DriveActionType::DownloadFile : {
					auto pActionDownloadFile = reinterpret_cast<const model::ActionDownloadFile*>(pAction);
					builder << "file";
					StreamFile(builder, pActionDownloadFile->File);
					break;
				}

				case model::DriveActionType::DeleteFile : {
					auto pActionDeleteFile = reinterpret_cast<const model::ActionDeleteFile*>(pAction);
					builder << "file";
					StreamFile(builder, pActionDeleteFile->File);
					break;
				}

				case model::DriveActionType::MoveFile : {
					auto* pActionMoveFile = reinterpret_cast<const model::ActionMoveFile *>(pAction);
					builder << "source";
					StreamFile(builder, pActionMoveFile->Source);
					builder << "destination";
					StreamFile(builder, pActionMoveFile->Destination);
					break;
				}

				case model::DriveActionType::CopyFile : {
					auto* pActionCopyFile = reinterpret_cast<const model::ActionCopyFile *>(pAction);
					builder << "source";
					StreamFile(builder, pActionCopyFile->Source);
					builder << "destination";
					StreamFile(builder, pActionCopyFile->Destination);
					break;
				}
			}

			builder << bson_stream::close_document;
		}
	}

	template<typename TTransaction>
	void StreamServiceTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "recipient" << ToBinary(transaction.Recipient);
		StreamMosaics(builder, transaction.MosaicsPtr(), transaction.MosaicsCount);
		StreamAction(builder, transaction.ActionType, transaction.ActionPtr());
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(Service, StreamServiceTransaction)
}}}
