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

#include "LockFundTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"

namespace catapult { namespace test {

	std::unordered_map<Key, state::LockFundRecordGroup<cache::LockFundKeyIndexDescriptor>, utils::ArrayHasher<Key>> DeriveKeyRecordsFromHeightRecord(state::LockFundRecordGroup<cache::LockFundHeightIndexDescriptor> record)
	{
		std::unordered_map<Key, state::LockFundRecordGroup<cache::LockFundKeyIndexDescriptor>, utils::ArrayHasher<Key>> results;
		for(auto lockFundRecord : record.LockFundRecords)
		{
			auto keyGroup = results.find(lockFundRecord.first);
			if(keyGroup != results.end())
			{
				auto heightRecord = keyGroup->second.LockFundRecords.find(record.Identifier);
				if(heightRecord != keyGroup->second.LockFundRecords.end())
				{
					if(lockFundRecord.second.Active())
						heightRecord->second.Set(lockFundRecord.second.Get());
					heightRecord->second.InactiveRecords = lockFundRecord.second.InactiveRecords;
				}
				else
					keyGroup->second.LockFundRecords.insert(std::make_pair(record.Identifier, lockFundRecord.second));
			}
			else
			{
				state::LockFundRecordGroup<cache::LockFundKeyIndexDescriptor> group;
				group.Identifier = lockFundRecord.first;
				group.LockFundRecords.insert(std::make_pair(record.Identifier, lockFundRecord.second));
				results.insert(std::make_pair(group.Identifier, group));
			}
		}
		return results;
	}
	std::shared_ptr<config::BlockchainConfigurationHolder> CreateLockFundConfigHolder() {
		auto pluginConfig = config::LockFundConfiguration::Uninitialized();
		pluginConfig.MinRequestUnlockCooldown = BlockDuration(200000);
		pluginConfig.MaxMosaicsSize = 256;
		auto networkConfig = model::NetworkConfiguration::Uninitialized();
		networkConfig.BlockGenerationTargetTime = utils::TimeSpan::FromHours(1);
		networkConfig.SetPluginConfiguration(pluginConfig);
		return config::CreateMockConfigurationHolder(networkConfig);
	}
	void AssertEqual(state::LockFundRecord originalRecord, state::LockFundRecord loadedRecord)
	{
		EXPECT_EQ(originalRecord.Size(), loadedRecord.Size());
		EXPECT_EQ(originalRecord.Active(), loadedRecord.Active());
		for(auto mosaic : originalRecord.Get())
		{
			EXPECT_EQ(loadedRecord.Get().find(mosaic.first)->second, mosaic.second);
		}
		for(auto inactiveRecord  : originalRecord.InactiveRecords)
		{
			int i = 0;
			for(auto mosaic : inactiveRecord)
			{
				EXPECT_EQ(loadedRecord.InactiveRecords[i].find(mosaic.first)->second, mosaic.second);
				i++;
			}
		}
	}



}}
