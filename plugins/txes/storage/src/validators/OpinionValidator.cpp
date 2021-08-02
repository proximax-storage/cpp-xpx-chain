/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <boost/dynamic_bitset.hpp>
#include "Validators.h"
#include "src/catapult/crypto/Signer.h"

namespace catapult { namespace validators {

	namespace {
		template<typename TPointer, typename TData>
		inline void WriteToArray(TPointer*& ptr, const TData& data) {
			const auto pData = reinterpret_cast<const TPointer*>(&data);
			std::copy(pData, pData + sizeof(data), ptr);
			ptr += sizeof(data);
		}
	}

	using Notification = model::DownloadApprovalNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(Opinion, ([](const Notification& notification, const ValidatorContext& context) {
		const auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();

	  	// Nth vector in blsPublicKeys contains pointers to all public BLS keys of replicators that provided Nth opinion.
		std::vector<std::vector<const BLSPublicKey*>> blsPublicKeys(notification.OpinionCount);

	  	auto pIndex = notification.OpinionIndicesPtr;
	  	for (auto i = 0; i < notification.JudgingCount; ++i, ++pIndex) {
			if (*pIndex >= notification.OpinionCount)
				return Failure_Storage_Invalid_Opinion_Index;
			const auto replicatorIter = replicatorCache.find(*(notification.PublicKeysPtr + i));
	  		const auto& replicatorEntry = replicatorIter.tryGet();
			if (!replicatorEntry)
				return Failure_Storage_Replicator_Not_Found;
			blsPublicKeys.at(*pIndex).push_back(&replicatorEntry->blsKey());
		}

	  	// Bitset that represents boolean array of size (notification.OpinionCount * notification.JudgedCount) of opinion presence.
	  	const auto presentOpinionByteCount = (notification.OpinionCount * notification.JudgedCount + 7) / 8;
	  	boost::dynamic_bitset<uint8_t> presentOpinions(notification.PresentOpinionsPtr, notification.PresentOpinionsPtr + presentOpinionByteCount);

	  	// Set that represents complete opinion of one of the replicators. Opinion elements are sorted in ascending order of keys.
		using OpinionElement = std::pair<Key, uint64_t>;
		const auto comparator = [](const OpinionElement& a, const OpinionElement& b){ return a.first < b.first; };
		std::set<OpinionElement, decltype(comparator)> individualData(comparator);

	  	// Preparing common data.
		// TODO: Move to DownloadApprovalTransactionPlugin
	  	const auto commonDataSize = sizeof(notification.DownloadChannelId)
									+ sizeof(notification.SequenceNumber)
									+ sizeof(notification.ResponseToFinishDownloadTransaction);
		const auto maxDataSize = commonDataSize + (sizeof(Key) + sizeof(uint64_t)) * notification.JudgedCount;	// Guarantees that every possible individual opinion will fit in.
		auto* const pCommonDataBegin = new uint8_t[maxDataSize];
	  	auto* pCommonData = pCommonDataBegin;
	  	WriteToArray(pCommonData, notification.DownloadChannelId);
	  	WriteToArray(pCommonData, notification.SequenceNumber);
	  	WriteToArray(pCommonData, notification.ResponseToFinishDownloadTransaction);
		auto* const pIndividualDataBegin = pCommonData;

		// Validating signatures.
	  	auto pBlsSignature = notification.BlsSignaturesPtr;
	  	auto pOpinionElement = notification.OpinionsPtr;
		for (auto i = 0; i < notification.OpinionCount; ++i, ++pBlsSignature) {
			individualData.clear();
			for (auto j = 0; j < notification.JudgedCount; ++j) {
				if (presentOpinions[i*notification.JudgedCount + j]) {
					individualData.emplace(*(notification.PublicKeysPtr + j), *pOpinionElement);
					++pOpinionElement;
				}
			}

			const auto dataSize = commonDataSize + (sizeof(Key) + sizeof(uint64_t)) * individualData.size();
			auto* pIndividualData = pIndividualDataBegin;
			for (auto individualDataIter = individualData.begin(); individualDataIter != individualData.end(); ++individualDataIter) {
				WriteToArray(pIndividualData, individualDataIter->first);
				WriteToArray(pIndividualData, individualDataIter->second);
			}

			RawBuffer dataBuffer(pCommonDataBegin, dataSize);

			if (!crypto::FastAggregateVerify(blsPublicKeys.at(i), dataBuffer, *pBlsSignature))
				return Failure_Storage_Invalid_BLS_Signature;
		}

		return ValidationResult::Success;
	}))

}}
