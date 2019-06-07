/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"

namespace catapult { namespace mocks {

	struct MockSupportedVersionSupplier {
		MockSupportedVersionSupplier(VersionSet supportedVersions) : m_supportedVersions(supportedVersions)
		{}

		const VersionSet& operator()() const {
			return m_supportedVersions;
		}

	private:
		VersionSet m_supportedVersions;
	};
}}