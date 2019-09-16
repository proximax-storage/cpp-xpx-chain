/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "ServiceTransactionPlugin.h"
#include "plugins/txes/multisig/src/model/MultisigNotifications.h"
#include "src/model/ServiceTransaction.h"
#include "src/model/ServiceNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		UnresolvedAddress CopyToUnresolvedAddress(const Address& address) {
			UnresolvedAddress dest;
			std::memcpy(dest.data(), address.data(), address.size());
			return dest;
		}

		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
					case 1: {
						sub.notify(AccountPublicKeyNotification<1>(transaction.Recipient));
						auto recipientAddress = CopyToUnresolvedAddress(PublicKeyToAddress(transaction.Recipient, config.NetworkIdentifier));
						sub.notify(AddressInteractionNotification<1>(transaction.Signer, transaction.Type, {recipientAddress}));

						const auto *pMosaics = transaction.MosaicsPtr();
						for (auto i = 0u; i < transaction.MosaicsCount; ++i) {
							auto notification = BalanceTransferNotification<1>(
								transaction.Signer,
								recipientAddress,
								pMosaics[i].MosaicId,
								pMosaics[i].Amount);
							sub.notify(notification);
						}

						if (transaction.MosaicsCount)
							sub.notify(TransferMosaicsNotification<1>(transaction.ActionType, transaction.MosaicsCount, pMosaics));

						if (transaction.ActionType != DriveActionType::PrepareDrive) {
							Key drive;
							switch (transaction.ActionType) {
								case DriveActionType::PrepareDrive :
								case DriveActionType::DriveProlongation :
								case DriveActionType::DriveDeposit :
								case DriveActionType::FileDeposit :
								case DriveActionType::DriveVerification :
								case DriveActionType::CreateDirectory :
								case DriveActionType::RemoveDirectory :
								case DriveActionType::UploadFile :
								case DriveActionType::DownloadFile :
								case DriveActionType::DeleteFile :
								case DriveActionType::MoveFile :
								case DriveActionType::CopyFile :
									drive = transaction.Recipient;
									break;;
								case DriveActionType::DriveDepositReturn :
								case DriveActionType::DrivePayment :
								case DriveActionType::FileDepositReturn :
								case DriveActionType::FilePayment :
									drive = transaction.Signer;
									break;
							}
							sub.notify(model::DriveNotification<1>(drive));
						}

						switch (transaction.ActionType) {
							case DriveActionType::PrepareDrive : {
								auto& mosaic = pMosaics[0];
								sub.notify(model::MosaicIdNotification<1>(mosaic.MosaicId, config.CurrencyMosaicId));
								auto* pAction = reinterpret_cast<const ActionPrepareDrive*>(transaction.ActionPtr());
								sub.notify(model::PrepareDriveNotification<1>(
									transaction.Recipient,
									pAction->Duration,
									pAction->Size,
									pAction->Replicas,
									transaction.Signer,
									mosaic));
								break;
							}

							case DriveActionType::DriveProlongation : {
								auto& mosaic = pMosaics[0];
								sub.notify(model::MosaicIdNotification<1>(mosaic.MosaicId, config.CurrencyMosaicId));
								auto* pAction = reinterpret_cast<const ActionDriveProlongation*>(transaction.ActionPtr());
								sub.notify(model::DriveProlongationNotification<1>(
									transaction.Recipient,
									pAction->Duration,
									transaction.Signer,
									mosaic));
								break;
							}

							case DriveActionType::DriveDeposit : {
								auto& mosaic = pMosaics[0];
								sub.notify(model::MosaicIdNotification<1>(mosaic.MosaicId, config.StorageMosaicId));
								sub.notify(ModifyMultisigNewCosignerNotification<1>(transaction.Recipient, transaction.Signer));
								sub.notify(model::DriveDepositNotification<1>(
									transaction.Recipient,
									transaction.Signer,
									mosaic));
								break;
							}

							case DriveActionType::DriveDepositReturn : {
								auto& mosaic = pMosaics[0];
								sub.notify(model::MosaicIdNotification<1>(mosaic.MosaicId, config.StorageMosaicId));
								sub.notify(model::DriveDepositReturnNotification<1>(
									transaction.Signer,
									transaction.Recipient,
									mosaic));
								break;
							}

							case DriveActionType::DrivePayment : {
								sub.notify(ReplicatorNotification<1>(transaction.Signer, transaction.Recipient));
								CosignatoryModification modification{CosignatoryModificationType::Del, transaction.Recipient};
								sub.notify(ModifyMultisigCosignersNotification<1>(transaction.Signer, 1, &modification));
								break;
							}

							case DriveActionType::FileDeposit : {
								auto& mosaic = pMosaics[0];
								sub.notify(model::MosaicIdNotification<1>(mosaic.MosaicId, config.StreamingMosaicId));
								sub.notify(ReplicatorNotification<1>(transaction.Recipient, transaction.Signer));
								auto* pAction = reinterpret_cast<const ActionFileDeposit*>(transaction.ActionPtr());
								sub.notify(model::FileDepositNotification<1>(
									transaction.Recipient,
									transaction.Signer,
									mosaic,
									pAction->FileHash));
								break;
							}

							case DriveActionType::FileDepositReturn : {
								auto& mosaic = pMosaics[0];
								sub.notify(model::MosaicIdNotification<1>(mosaic.MosaicId, config.StreamingMosaicId));
								sub.notify(ReplicatorNotification<1>(transaction.Signer, transaction.Recipient));
								auto* pAction = reinterpret_cast<const ActionFileDepositReturn*>(transaction.ActionPtr());
								sub.notify(model::FileDepositReturnNotification<1>(
									transaction.Recipient,
									transaction.Signer,
									mosaic,
									pAction->FileHash));
								break;
							}

							case DriveActionType::FilePayment : {
								sub.notify(ReplicatorNotification<1>(transaction.Signer, transaction.Recipient));
								break;
							}

							case DriveActionType::DriveVerification : {
								auto& mosaic = pMosaics[0];
								sub.notify(model::MosaicIdNotification<1>(mosaic.MosaicId, config.StorageMosaicId));
								sub.notify(ReplicatorNotification<1>(transaction.Recipient, transaction.Signer));
								sub.notify(model::DriveVerificationNotification<1>(
									transaction.Recipient,
									transaction.Signer,
									mosaic));
								break;
							}

							case DriveActionType::CreateDirectory : {
								auto* pAction = reinterpret_cast<const ActionCreateDirectory*>(transaction.ActionPtr());
								sub.notify(model::CreateDirectoryNotification<1>(
									transaction.Recipient,
									pAction->Directory));
								break;
							}

							case DriveActionType::RemoveDirectory : {
								auto* pAction = reinterpret_cast<const ActionRemoveDirectory*>(transaction.ActionPtr());
								sub.notify(model::RemoveDirectoryNotification<1>(
									transaction.Recipient,
									pAction->Directory));
								break;
							}

							case DriveActionType::UploadFile : {
								auto& mosaic = pMosaics[0];
								sub.notify(model::MosaicIdNotification<1>(mosaic.MosaicId, config.StreamingMosaicId));
								auto* pAction = reinterpret_cast<const ActionUploadFile*>(transaction.ActionPtr());
								sub.notify(model::UploadFileNotification<1>(
									transaction.Recipient,
									pAction->File));
								break;
							}

							case DriveActionType::DownloadFile : {
								auto& mosaic = pMosaics[0];
								sub.notify(model::MosaicIdNotification<1>(mosaic.MosaicId, config.StreamingMosaicId));
								sub.notify(ReplicatorNotification<1>(transaction.Recipient, transaction.Signer));
								auto* pAction = reinterpret_cast<const ActionDownloadFile*>(transaction.ActionPtr());
								sub.notify(model::FileDepositNotification<1>(
									transaction.Recipient,
									transaction.Signer,
									mosaic,
									pAction->File.Hash));
								sub.notify(model::DownloadFileNotification<1>(
									transaction.Recipient,
									pAction->File));
								break;
							}

							case DriveActionType::DeleteFile : {
								auto* pAction = reinterpret_cast<const ActionDeleteFile*>(transaction.ActionPtr());
								sub.notify(model::DeleteFileNotification<1>(
									transaction.Recipient,
									pAction->File));
								break;
							}

							case DriveActionType::MoveFile : {
								auto& mosaic = pMosaics[0];
								sub.notify(model::MosaicIdNotification<1>(mosaic.MosaicId, config.CurrencyMosaicId));
								auto* pAction = reinterpret_cast<const ActionMoveFile*>(transaction.ActionPtr());
								sub.notify(model::MoveFileNotification<1>(
									transaction.Recipient,
									pAction->Source,
									pAction->Destination));
								break;
							}

							case DriveActionType::CopyFile : {
								auto& mosaic = pMosaics[0];
								sub.notify(model::MosaicIdNotification<1>(mosaic.MosaicId, config.CurrencyMosaicId));
								auto* pAction = reinterpret_cast<const ActionCopyFile*>(transaction.ActionPtr());
								sub.notify(model::CopyFileNotification<1>(
									transaction.Recipient,
									pAction->Source,
									pAction->Destination));
								break;
							}
						}

						break;
					}

					default:
						CATAPULT_LOG(debug) << "invalid version of ServiceTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(Service, Default, CreatePublisher, config::ImmutableConfiguration)
}}
