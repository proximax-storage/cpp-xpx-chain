/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <boost/dynamic_bitset.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::DownloadApprovalPaymentNotification<1>;

	using BigUint = boost::multiprecision::uint128_t;

	DEFINE_OBSERVER_WITH_LIQUIDITY_PROVIDER(DownloadApprovalPayment, Notification, ([&liquidityProvider](const Notification& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DownloadApprovalPayment)");

	  	auto& downloadChannelCache = context.Cache.sub<cache::DownloadChannelCache>();
	  	auto downloadChannelIter = downloadChannelCache.find(notification.DownloadChannelId);
	  	auto& downloadChannelEntry = downloadChannelIter.get();

		const auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto senderIter = accountStateCache.find(Key(notification.DownloadChannelId.array()));
	  	const auto& senderState = senderIter.get();

	  	const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
		const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
		auto& statementBuilder = context.StatementBuilder();

		// Maps each replicator key to a vector of opinions about that replicator.
		std::map<Key, std::vector<uint64_t>> opinions;

		// Bitset that represents boolean array of size (transaction.OpinionCount * totalJudgedKeysCount) of opinion presence.
	  	const auto totalJudgingKeysCount = notification.JudgingKeysCount + notification.OverlappingKeysCount;
		const auto totalJudgedKeysCount = notification.OverlappingKeysCount + notification.JudgedKeysCount;
		const auto presentOpinionByteCount = (totalJudgingKeysCount * totalJudgedKeysCount + 7) / 8;
		boost::dynamic_bitset<uint8_t> presentOpinions(notification.PresentOpinionsPtr, notification.PresentOpinionsPtr + presentOpinionByteCount);

		// Filling in opinions map.
		auto pOpinionElement = notification.OpinionsPtr;
		for (auto i = 0; i < totalJudgingKeysCount; ++i)
			for (auto j = 0; j < totalJudgedKeysCount; ++j) {
				const auto& replicatorKey = notification.PublicKeysPtr[notification.JudgingKeysCount + j];
				auto& opinionVector = opinions[replicatorKey];
				opinionVector.emplace_back(presentOpinions[i*totalJudgedKeysCount + j] ?
			 			*pOpinionElement++ :
					   	downloadChannelEntry.cumulativePayments().at(replicatorKey).unwrap());
			}

		// Calculating full payments to the replicators based on median opinions about them.
		std::map<Key, uint64_t> payments;
		uint64_t totalPayment = 0;
		for (auto& pair: opinions) {
			auto& opinionVector = pair.second;
			std::sort(opinionVector.begin(), opinionVector.end());
			const auto medianIndex = opinionVector.size() / 2;	// Corresponds to upper median index when the size is even
			const auto medianOpinion = opinionVector.size() % 2 ?
									   opinionVector.at(medianIndex) :
									   (opinionVector.at(medianIndex-1) + opinionVector.at(medianIndex)) / 2;
			uint64_t fullPayment = 0;
			if (medianOpinion > downloadChannelEntry.cumulativePayments().at(pair.first).unwrap()) {
				fullPayment = medianOpinion - downloadChannelEntry.cumulativePayments().at(pair.first).unwrap();
			}
			payments[pair.first] = fullPayment;
			totalPayment += fullPayment;
		}

		// Scaling down payments if there's not enough mosaics on the download channel for full payments to all of the replicators.
		const auto& downloadChannelBalance = senderState.Balances.get(streamingMosaicId).unwrap();
		if (downloadChannelBalance < totalPayment) {
			for (auto& [_, payment]: payments) {
				auto paymentLong = BigUint(payment);
				payment = ((paymentLong * downloadChannelBalance) / totalPayment).convert_to<uint64_t>();
			}
		}

		// Making mosaic transfers and updating cumulative payments.
		for (const auto& [replicatorKey, bytesPayment]: payments) {
			auto recipientIter = accountStateCache.find(replicatorKey);
			auto& recipientState = recipientIter.get();
			const auto megabytesPayment = utils::FileSize::FromBytes(bytesPayment).megabytes();
			liquidityProvider->debitMosaics(context, downloadChannelEntry.id().array(), replicatorKey, config::GetUnresolvedStreamingMosaicId(context.Config.Immutable), Amount(megabytesPayment));

			// Adding Download Approval receipt.
			const auto receiptType = model::Receipt_Type_Download_Approval;
			const model::StorageReceipt receipt(receiptType, downloadChannelEntry.id().array(), replicatorKey,
												{ streamingMosaicId, currencyMosaicId }, Amount(megabytesPayment));
			statementBuilder.addTransactionReceipt(receipt);

			auto& cumulativePayment = downloadChannelEntry.cumulativePayments().at(replicatorKey);
			cumulativePayment = cumulativePayment + Amount(bytesPayment);
		}
	}))
}}
