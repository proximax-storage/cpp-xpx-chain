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
#include "catapult/types.h"
#include <optional>
namespace catapult { namespace state {

	using LockFundRecordMosaicMap = std::map<MosaicId, Amount>;
	/// A container that contains an active lock fund record mosaic map and an ordered list of inactive ones.
	struct LockFundRecord
	{
		LockFundRecord() = default;
		LockFundRecord(LockFundRecordMosaicMap record)
		: ActiveRecord(record)
		{
		}
		LockFundRecord(LockFundRecordMosaicMap record, std::vector<LockFundRecordMosaicMap>&& inactiveRecords)
				: ActiveRecord(record)
				, InactiveRecords(std::move(inactiveRecords))
		{}

		/// Checks whether a record is active
		bool Active() const
		{
			return ActiveRecord.has_value();
		}
		/// Gets the current value
		LockFundRecordMosaicMap& Get()
		{
			return ActiveRecord.value();
		}

		/// Gets the current value
		const LockFundRecordMosaicMap& Get() const
		{
			return ActiveRecord.value();
		}

		/// Gets the number of inactive records
		size_t Size() const
		{
			return InactiveRecords.size();
		}

		/// Checks whether there Inactive Records vector is empty
		bool Empty() const
		{
			return InactiveRecords.empty();
		}

		/// Sets the current active record
		void Set(const LockFundRecordMosaicMap& record)
		{
			ActiveRecord.emplace(record);
		}

		/// Removes the current active record
		void Unset()
		{
			ActiveRecord.reset();
		}

		/// Sets the current active record
		void Set(LockFundRecordMosaicMap&& record)
		{
			ActiveRecord.emplace(std::move(record));
		}

		/// Inactivates the current record moving it to the inactive records stack
		void Inactivate()
		{
			InactiveRecords.emplace_back(std::move(ActiveRecord.value()));
			ActiveRecord.reset();
		}

		/// Reactives the last inactivated record
		void Reactivate()
		{
			ActiveRecord.emplace(std::move(InactiveRecords.back()));
			InactiveRecords.pop_back();
		}
		std::vector<LockFundRecordMosaicMap> InactiveRecords;

	private:
		std::optional<LockFundRecordMosaicMap> ActiveRecord;
	};

}}
