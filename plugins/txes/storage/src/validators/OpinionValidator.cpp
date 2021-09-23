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

	  	// Nth vector in blsPublicKeys contains pointers to all public BLS keys of replicators that provided Nth opinion.
		std::vector<std::vector<const BLSPublicKey*>> blsPublicKeys(notification.OpinionCount);

	  	// Preparing blsPublicKeys.
		auto pIndex = notification.OpinionIndicesPtr;
	  	for (auto i = 0; i < notification.JudgingCount; ++i, ++pIndex) {
			if (*pIndex >= notification.OpinionCount)
				return Failure_Storage_Invalid_Opinion_Index;
			const auto replicatorIter = replicatorCache.find(notification.PublicKeysPtr[i]);
	  		const auto& pReplicatorEntry = replicatorIter.tryGet();
			if (!pReplicatorEntry)
				return Failure_Storage_Replicator_Not_Found;
			blsPublicKeys.at(*pIndex).push_back(&pReplicatorEntry->blsKey());
		}

	  	// Bitset that represents boolean array of size (notification.OpinionCount * notification.JudgedCount) of opinion presence.
	  	const auto presentOpinionByteCount = (notification.OpinionCount * notification.JudgedCount + 7) / 8;
	  	boost::dynamic_bitset<uint8_t> presentOpinions(notification.PresentOpinionsPtr, notification.PresentOpinionsPtr + presentOpinionByteCount);

		// Validating that each provided public key
	  	// - is unique
		// - is used in at least one opinion (i.e. that each column of PresentOpinions has at least one set bit)
		std::set<Key> providedKeys;
		auto pKey = notification.PublicKeysPtr;
		for (auto i = 0; i < notification.JudgedCount; ++i, ++pKey) {
			if (providedKeys.count(*pKey))
				return Failure_Storage_Opinion_Reocurring_Keys;
			providedKeys.insert(*pKey);
			bool isUsed = false;
			for (auto j = 0; !isUsed && j < notification.OpinionCount; ++j)
				isUsed = presentOpinions[j*notification.JudgedCount + i];
			if (!isUsed)
				return Failure_Storage_Opinion_Unused_Key;
		}

	  	// Preparing common data.
	  	const auto maxDataSize = notification.CommonDataSize + (sizeof(Key) + sizeof(uint64_t)) * notification.JudgedCount;	// Guarantees that every possible individual opinion will fit in.
		auto* const pDataBegin = new uint8_t[maxDataSize];
	  	std::copy(notification.CommonDataPtr, notification.CommonDataPtr + notification.CommonDataSize, pDataBegin);
	  	auto* const pIndividualDataBegin = pDataBegin + notification.CommonDataSize;

		// Validating signatures.
	  	auto pBlsSignature = notification.BlsSignaturesPtr;
	  	auto pOpinionElement = notification.OpinionsPtr;
	  	using OpinionElement = std::pair<Key, uint64_t>;
		const auto comparator = [](const OpinionElement& a, const OpinionElement& b){ return a.first < b.first; };
		using IndividualPart = std::set<OpinionElement, decltype(comparator)>;
	  	IndividualPart individualPart(comparator);	// Set that represents complete opinion of one of the replicators. Opinion elements are sorted in ascending order of keys.
		std::set<IndividualPart> providedIndividualParts;	// Set of provided complete opinions. Used to determine if there are reoccurring individual parts.
		for (auto i = 0; i < notification.OpinionCount; ++i, ++pBlsSignature) {
			individualPart.clear();
			for (auto j = 0; j < notification.JudgedCount; ++j) {
				if (presentOpinions[i*notification.JudgedCount + j]) {
					individualPart.emplace(notification.PublicKeysPtr[j], *pOpinionElement);
					++pOpinionElement;
				}
			}

			if (providedIndividualParts.count(individualPart)) {
				delete[] pDataBegin;
				return Failure_Storage_Opinions_Reocurring_Individual_Parts;
			}
			providedIndividualParts.insert(individualPart);

			const auto dataSize = notification.CommonDataSize + (sizeof(Key) + sizeof(uint64_t)) * individualPart.size();
			auto* pIndividualData = pIndividualDataBegin;
			for (auto individualPartIter = individualPart.begin(); individualPartIter != individualPart.end(); ++individualPartIter) {
				utils::WriteToByteArray(pIndividualData, individualPartIter->first);
				utils::WriteToByteArray(pIndividualData, individualPartIter->second);
			}

			RawBuffer dataBuffer(pDataBegin, dataSize);

			if (!crypto::FastAggregateVerify(blsPublicKeys.at(i), dataBuffer, *pBlsSignature)) {
				delete[] pDataBegin;
				return Failure_Storage_Invalid_BLS_Signature;
			}
		}

		delete[] pDataBegin;
		return ValidationResult::Success;
	}))

}}
