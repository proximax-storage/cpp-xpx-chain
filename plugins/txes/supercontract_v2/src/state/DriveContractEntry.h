/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <catapult/types.h>

namespace catapult::state {

// Mixin for matching drive key with contract key.
class DriveContractMixin {
public:
	DriveContractMixin() {}

	const Key& contractKey() const {
		return m_contractKey;
	}
	void setContractKey(const Key& contractKey) {
		m_contractKey = contractKey;
	}

private:
	Key m_contractKey;
};

// Drivecontract entry.
class DriveContractEntry : public DriveContractMixin {
public:
	// Creates a super contract entry around \a key.
	explicit DriveContractEntry(const Key& key) : m_key(key) {}

public:
	// Gets the drive contract public key.
	const Key& key() const {
		return m_key;
	}

private:
	Key m_key;
};
} // namespace catapult::state