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
#include <optional>
namespace catapult { namespace state {

	using LockFundRecord = std::map<MosaicId, Amount>;
	/// A container that contains an active lock fund record and an ordered list of inactive ones.
	struct LockFundRecordContainer
	{
		LockFundRecordContainer() = default;
		LockFundRecordContainer(LockFundRecord record)
		: ActiveRecord(record)
		{
		}
		LockFundRecordContainer(LockFundRecord record, std::vector<LockFundRecord>&& inactiveRecords)
				: ActiveRecord(record)
				, InactiveRecords(std::move(inactiveRecords))
		{}

		/// Inactivates the current record moving it to the inactive records stack
		void Inactivate()
		{
			InactiveRecords.emplace(std::move(ActiveRecord.value()));
			ActiveRecord.reset();
		}

		/// Reactives the last inactivated record
		void Reactivate()
		{
			ActiveRecord.emplace(std::move(InactiveRecords.top()));
			InactiveRecords.pop();
		}
		std::optional<LockFundRecord> ActiveRecord;
		std::stack<LockFundRecord, std::vector<LockFundRecord>> InactiveRecords;
	};
	/// A lock fund record.
	template<typename TIndexDescriptor>
	using LockFundRecordMap = std::unordered_map<typename TIndexDescriptor::ValueIdentifier, LockFundRecordContainer, typename TIndexDescriptor::KeyHashFunction>;
}}
