/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LockFundRecord.h"
#include <unordered_map>
#include "catapult/utils/Hashers.h"

namespace catapult { namespace state {

	/// A lock fund record.
	template<typename TIndexDescriptor>
	using LockFundRecordMap = std::unordered_map<typename TIndexDescriptor::ValueIdentifier, LockFundRecord, typename TIndexDescriptor::KeyHashFunction>;

	template<typename TKeyType, typename TValueIdentifier, typename TKeyHashFunction>
	struct LockFundRecordGroupIndexDescriptor{
		using KeyType = TKeyType;
		using ValueIdentifier = TValueIdentifier;
		using KeyHashFunction = TKeyHashFunction;
	};

	/// Descriptors
	/// Descriptor for height indexed records.
	using LockFundHeightIndexDescriptor = LockFundRecordGroupIndexDescriptor<Height, Key, utils::ArrayHasher<Key>>;

	/// Descriptor for key indexed records.
	using LockFundKeyIndexDescriptor = LockFundRecordGroupIndexDescriptor<Key, Height, utils::BaseValueHasher<Height>>;

	/// A lock fund record group identified by TIdentifier.
	template<typename TIndexDescriptor>
	struct LockFundRecordGroup {
	public:
		using Descriptor = TIndexDescriptor;
		/// Creates a default hash lock info.
		LockFundRecordGroup()
		{}

		/// Creates a Lock Fund Record Group around \a identifier and \a records.
		LockFundRecordGroup(
				const typename TIndexDescriptor::KeyType& identifier,
				const LockFundRecordMap<TIndexDescriptor>& records)
				: LockFundRecords(records),
				Identifier(identifier)
		{}

	public:
		/// Identifier
		typename TIndexDescriptor::KeyType Identifier;

		/// Associated LockFundRecords
		LockFundRecordMap<TIndexDescriptor> LockFundRecords;
	};
}}
