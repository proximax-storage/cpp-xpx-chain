/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once

#include <map>
#include "LockFundRecord.h"
namespace catapult { namespace state {

	template<typename TKeyType, typename TValueIdentifier, typename TKeyHashFunction>
	struct LockFundRecordGroupIndexDescriptor{
		using KeyType = TKeyType;
		using ValueIdentifier = TValueIdentifier;
		using KeyHashFunction = TKeyHashFunction;
	};
	/// A lock fund record group identified by TIdentifier.
	template<typename TIndexDescriptor>
	struct LockFundRecordGroup {
	public:
		/// Creates a default hash lock info.
		LockFundRecordGroup()
		{}

		/// Creates a hash lock info around \a account, \a mosaicId, \a amount, \a height and \a hash.
		LockFundRecordGroup(
				typename TIndexDescriptor::KeyType identifier,
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
