/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/state/DriveStateBrowser.h"
#include "catapult/cache/CatapultCache.h"
#include "catapult/state/ContractState.h"

namespace catapult { namespace state {

class ContractStateImpl : public ContractState {
public:

	explicit ContractStateImpl(const std::unique_ptr<DriveStateBrowser>& driveStateBrowser);

public:

	bool contractExists(const Key& contractKey) const override;
	std::shared_ptr<const model::BlockElement> getBlock(Height height) const override;
	std::optional<Height> getAutomaticExecutionsEnabledSince(const Key& contractKey) const override;
	Hash256 getDriveState(const Key& contractKey) const override;
	std::set<Key> getContracts(const Key& executorKey) const override;
	std::map<Key, ExecutorDigest> getExecutors(const Key& contractKey) const override;
	ContractInfo getContractInfo(const Key& contractKey) const override;

private:

	template<class T>
	auto getCacheView() const {
		return m_pCache->sub<T>().createView(m_pCache->height());
	}

private:

	const std::unique_ptr<DriveStateBrowser>& m_pDriveStateBrowser;

};

}}
