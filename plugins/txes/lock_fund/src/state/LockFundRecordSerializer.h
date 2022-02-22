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
#include "catapult/io/Stream.h"
#include "LockFundRecordGroup.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	namespace {
		void WriteLockFundRecord(io::OutputStream& output, const LockFundRecordMosaicMap& record)
		{
			io::Write8(output, utils::checked_cast<size_t, uint8_t>(record.size()));
			for (const auto& mosaicPair : record) {
				io::Write(output, mosaicPair.first);
				io::Write(output, mosaicPair.second);
			}
		}

		void LoadLockFundRecord(io::InputStream& input, LockFundRecordMosaicMap& record)
		{
			auto mosaicCount = io::Read8(input);
			while (mosaicCount--) {
				auto mosaicId = MosaicId{io::Read64(input)};
				auto amount = Amount{io::Read64(input)};
				record.emplace(mosaicId, amount);
			}
		}

		LockFundRecordMosaicMap LoadLockFundRecord(io::InputStream& input)
		{
			LockFundRecordMosaicMap record;
			auto mosaicCount = io::Read8(input);
			while (mosaicCount--) {
				auto mosaicId = MosaicId{io::Read64(input)};
				auto amount = Amount{io::Read64(input)};
				record.emplace(mosaicId, amount);
			}
			return std::move(record);
		}
	}
	/// Policy for saving and loading lock fund data.
	template<typename TIndexDescriptor>
	struct LockFundRecordSerializer {

		/// Saves \a lock fund record to \a output.
		static void Save(const LockFundRecordGroup<TIndexDescriptor>& lockFundRecordGroup, io::OutputStream& output)
		{
			// write version
			io::Write32(output, 1);
			io::Write(output, lockFundRecordGroup.Identifier);
			io::Write32(output, lockFundRecordGroup.LockFundRecords.size());
			for(const std::pair<typename TIndexDescriptor::ValueIdentifier, LockFundRecord>& lockFundRecordPair : lockFundRecordGroup.LockFundRecords)
			{
				io::Write(output, lockFundRecordPair.first);
				auto hasActiveRecord = lockFundRecordPair.second.Active();
				io::Write8(output, (uint8_t)hasActiveRecord);
				if(hasActiveRecord)
					WriteLockFundRecord(output, lockFundRecordPair.second.Get());
				io::Write32(output, lockFundRecordPair.second.Size());
				for (const auto& record : lockFundRecordPair.second.InactiveRecords) {
					WriteLockFundRecord(output, record);
				}
			}
		}

		/// Loads lock fund record from \a input into \a lockFundRecord.
		static LockFundRecordGroup<TIndexDescriptor> Load(io::InputStream& input)
		{
			// read version
			LockFundRecordGroup<TIndexDescriptor> lockFundRecordGroup;
			VersionType version = io::Read32(input);
			if (version > 1)
				CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of LockFundRecord", version);
			io::Read(input, lockFundRecordGroup.Identifier);
			auto size = io::Read32(input);
			while (size--)
			{
				typename TIndexDescriptor::ValueIdentifier identifier;
				io::Read(input, identifier);
				LockFundRecord recordContainer;
				bool isActive = io::Read8(input);
				if(isActive)
				{
					recordContainer.Set(LoadLockFundRecord(input));
				}

				std::pair<typename TIndexDescriptor::ValueIdentifier,LockFundRecord> entry(identifier, std::move(recordContainer));

				auto inactiveSize = io::Read32(input);
				while (inactiveSize--) {
					entry.second.InactiveRecords.emplace_back(LoadLockFundRecord(input));
				}
				lockFundRecordGroup.LockFundRecords.insert(std::move(entry));
			}
			return lockFundRecordGroup;
		}
	};
}}
