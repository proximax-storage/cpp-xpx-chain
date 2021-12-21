/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <boost/dynamic_bitset.hpp>
#include "sdk/src/extensions/ConversionExtensions.h"
#include "EndDriveVerificationTransactionPlugin.h"
#include "src/model/InternalStorageNotifications.h"
#include "src/model/EndDriveVerificationTransaction.h"
#include "src/utils/StorageUtils.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

    namespace {
		template<typename TTransaction>
		void PrepareOpinionNotificationFields(
				const TTransaction& transaction,
				uint8_t& judgingKeysCount,
				Signature* signaturesPtr,
				uint8_t* presentOpinionsPtr,
				uint8_t* opinionsPtr) {
			std::vector<Signature> signatures(transaction.VerificationOpinionsCount);
			std::vector<std::vector<uint8_t>> opinions;	// Full (not jagged) 2D array of opinions
			opinions.resize(transaction.VerificationOpinionsCount);
			const uint8_t NONE = 255;	// Special value to denote the absence of opinion

			// Fill in opinions and signatures:
			auto pVerificationOpinion = transaction.VerificationOpinionsPtr();
			for (auto i = 0u; i < transaction.VerificationOpinionsCount; ++i, ++pVerificationOpinion) {
				const auto& verifierIndex = pVerificationOpinion->Verifier;
				signatures.at(verifierIndex) = pVerificationOpinion->Signature;
				opinions.at(verifierIndex).resize(transaction.ProversCount, NONE);	// Initialize the row with absent values
				for (const auto& result : pVerificationOpinion->Results)
					opinions.at(verifierIndex).at(result.first) = result.second;
			}

			// Deduce judgingKeysCount:
			judgingKeysCount = 0;
			for (auto j = 0u; j < transaction.ProversCount; ++j) {	// Iterate over columns
				bool columnIsEmpty = true;
				for (auto i = 0u; i < transaction.VerificationOpinionsCount; ++i)	// Iterate over rows
					if (opinions.at(i).at(j) != NONE) {
						columnIsEmpty = false;
						break;
					}
				if (columnIsEmpty)
					++judgingKeysCount;
				else
					break;
			}

			// Fill in signaturesPtr:
			for (auto i = 0u; i < transaction.VerificationOpinionsCount; ++i)
				signaturesPtr[i] = signatures.at(i);

			// Fill in presentOpinionsPtr and opinionsPtr:
			const auto totalJudgedKeysCount = transaction.ProversCount - judgingKeysCount;
			const auto presentOpinionElementCount = transaction.VerificationOpinionsCount * totalJudgedKeysCount;
			const auto presentOpinionByteCount = (presentOpinionElementCount + 7) / 8;
			auto pOpinion = opinionsPtr;
			for (auto i = 0u; i < presentOpinionByteCount; ++i) {
				boost::dynamic_bitset<uint8_t> byte(8, 0u);
				for (auto j = 0u; j < std::min(8u, presentOpinionElementCount - i*8); ++j) {
					const auto bitNumber = i*8 + j;
					const auto& opinion = opinions.at(bitNumber / totalJudgedKeysCount).at(judgingKeysCount + bitNumber % totalJudgedKeysCount);
					byte[j] = opinion != NONE;
					if (byte[j]) {
						*pOpinion = opinion;
						++pOpinion;
					}
				}
				boost::to_block_range(byte, presentOpinionsPtr + i);
			}
		}

        template<typename TTransaction>
        auto CreatePublisher(const config::ImmutableConfiguration& config) {
            return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
                switch (transaction.EntityVersion()) {
                    case 1: {
                        sub.notify(EndDriveVerificationNotification<1>(
                                transaction.DriveKey,
                                transaction.VerificationTrigger,
                                transaction.ProversCount,
                                transaction.ProversPtr(),
                                transaction.VerificationOpinionsCount,
                                transaction.VerificationOpinionsPtr()
                        ));

						const auto commonDataSize = sizeof(transaction.DriveKey) + sizeof(transaction.VerificationTrigger);
						const auto commonDataPtr = std::make_unique<uint8_t[]>(commonDataSize);
						auto* pCommonData = commonDataPtr.get();
						utils::WriteToByteArray(pCommonData, transaction.DriveKey);
						utils::WriteToByteArray(pCommonData, transaction.VerificationTrigger);

						uint8_t judgingKeysCount;
						const auto signaturesPtr = std::make_unique<Signature[]>(transaction.VerificationOpinionsCount);	// Exact size
						const auto maxOpinionElementCount = transaction.VerificationOpinionsCount * transaction.ProversCount;
						const auto presentOpinionsPtr = std::make_unique<uint8_t[]>((maxOpinionElementCount + 7) / 8);	// Potentially excessive size
						const auto opinionsPtr = std::make_unique<uint8_t[]>(maxOpinionElementCount);	// Potentially excessive size

						PrepareOpinionNotificationFields(transaction, judgingKeysCount,
								signaturesPtr.get(), presentOpinionsPtr.get(), opinionsPtr.get());

						sub.notify(OpinionNotification<1>(
								commonDataSize,
								judgingKeysCount,
								transaction.VerificationOpinionsCount - judgingKeysCount,
								transaction.ProversCount - transaction.VerificationOpinionsCount,
								sizeof(uint8_t),
								commonDataPtr.get(),
								transaction.ProversPtr(),
								signaturesPtr.get(),
								presentOpinionsPtr.get(),
								reinterpret_cast<const uint8_t*>(opinionsPtr.get())
						));

                        break;
                    }

                    default:
                        CATAPULT_LOG(debug) << "invalid version of EndDriveVerificationTransaction: "
                                            << transaction.EntityVersion();
                }
            };
        }
    }

	std::unique_ptr<TransactionPlugin>
			CreateEndDriveVerificationTransactionPlugin(const config::ImmutableConfiguration& config) {
		using Factory = TransactionPluginFactory<TransactionPluginFactoryOptions::Default>;
		return Factory::Create<
				EndDriveVerificationTransaction,
				EmbeddedEndDriveVerificationTransaction,
				ExtendedEmbeddedEndDriveVerificationTransaction>(
				CreatePublisher<EndDriveVerificationTransaction>(config),
				CreatePublisher<ExtendedEmbeddedEndDriveVerificationTransaction>(config));
	}
}}
