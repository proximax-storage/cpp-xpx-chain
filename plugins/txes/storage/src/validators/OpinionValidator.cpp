/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <boost/dynamic_bitset.hpp>
#include "Validators.h"
#include "src/catapult/crypto/Signer.h"
#include "src/utils/StorageUtils.h"

namespace catapult { namespace validators {

	using Notification = model::OpinionNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(Opinion, ([](const Notification& notification, const ValidatorContext& context) {
		const auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
	  	const auto totalKeysCount = notification.JudgingKeysCount + notification.OverlappingKeysCount + notification.JudgedKeysCount;
	  	const auto totalJudgingKeysCount = totalKeysCount - notification.JudgedKeysCount;
	  	const auto totalJudgedKeysCount = totalKeysCount - notification.JudgingKeysCount;

	  	// Bitset that represents boolean array of size (totalJudgingKeysCount * totalJudgedKeysCount) of opinion presence.
	  	const auto presentOpinionByteCount = (totalJudgingKeysCount * totalJudgedKeysCount + 7) / 8;

	  	std::optional<boost::dynamic_bitset<uint8_t>> presentOpinionsHolder;

	  	if ( notification.PresentOpinionsPtr ) {
	  		presentOpinionsHolder = boost::dynamic_bitset<uint8_t>(notification.PresentOpinionsPtr, notification.PresentOpinionsPtr + presentOpinionByteCount);
		}

		// Validating that each provided public key
	  	// - is unique
		// - is used in at least one opinion, if belongs to judged keys (i.e. that each column of PresentOpinions has at least one set bit)
		std::set<Key> providedKeys;
		for (auto i = 0; i < totalKeysCount; ++i) {
			const auto key = notification.PublicKeysPtr[i];
			if (!providedKeys.insert(key).second)
				return Failure_Storage_Opinion_Duplicated_Keys;
		}

		if ( presentOpinionsHolder ) {
			const auto& presentOpinions = *presentOpinionsHolder;

			for (auto i = notification.JudgingKeysCount; i < totalKeysCount; ++i) {
				bool isUsed = false;
				for (auto j = 0; !isUsed && j < totalJudgingKeysCount; ++j)
					isUsed = presentOpinions[j * totalJudgedKeysCount + i - notification.JudgingKeysCount];
				if (!isUsed)
					return Failure_Storage_Opinion_Unused_Key;
			}
		}

	  	// Preparing common data.
		std::vector<uint8_t> buffer(notification.CommonDataSize + (Key_Size + notification.OpinionElementSize) * totalJudgedKeysCount); // Guarantees that every possible individual opinion will fit in.
	  	std::copy(notification.CommonDataPtr, notification.CommonDataPtr + notification.CommonDataSize, buffer.data());
	  	auto* const pIndividualDataBegin = buffer.data() + notification.CommonDataSize;

		// Validating signatures.
	  	auto pKey = notification.PublicKeysPtr;
	  	auto pSignature = notification.SignaturesPtr;
	  	auto pOpinionElement = notification.OpinionsPtr;
	  	using OpinionElement = std::pair<Key, const uint8_t*>;

		for (auto i = 0; i < totalJudgingKeysCount; ++i, ++pKey, ++pSignature) {
			uint64_t dataSize = 0;
			if ( presentOpinionsHolder ) {
				const auto comparator = [](const OpinionElement& a, const OpinionElement& b){ return a.first < b.first; };
				using IndividualPart = std::set<OpinionElement, decltype(comparator)>;
				IndividualPart individualPart(comparator);	// Set that represents complete opinion of one of the replicators. Opinion elements are sorted in ascending order of keys.

				const auto& presentOpinions = *presentOpinionsHolder;

				for (auto j = 0; j < totalJudgedKeysCount; ++j) {
					if (presentOpinions[i * totalJudgedKeysCount + j]) {
						individualPart.emplace(notification.PublicKeysPtr[notification.JudgingKeysCount + j], pOpinionElement);
						pOpinionElement += notification.OpinionElementSize;
					}
				}

				dataSize = notification.CommonDataSize + (Key_Size + notification.OpinionElementSize) * individualPart.size();

				auto* pIndividualData = pIndividualDataBegin;
				for (const auto& pair : individualPart) {
					utils::WriteToByteArray(pIndividualData, pair.first);
					pIndividualData = std::copy(pair.second, pair.second + notification.OpinionElementSize, pIndividualData);
				}
			} else {
				std::copy(pOpinionElement, pOpinionElement + notification.OpinionElementSize * totalJudgedKeysCount, pIndividualDataBegin);
				pOpinionElement += notification.OpinionElementSize * totalJudgedKeysCount;

				dataSize = notification.CommonDataSize + notification.OpinionElementSize * totalJudgedKeysCount;

			}

			RawBuffer dataBuffer(buffer.data(), dataSize);

			if (!crypto::Verify(*pKey, dataBuffer, *pSignature))
				return Failure_Storage_Opinion_Invalid_Signature;
		}

		return ValidationResult::Success;
	}))

}}
