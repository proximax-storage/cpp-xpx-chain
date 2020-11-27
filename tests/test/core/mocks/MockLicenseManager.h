/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/licensing/LicenseManager.h"

namespace catapult { namespace mocks {

	struct MockLicenseManager : public licensing::LicenseManager {
	public:
		MockLicenseManager(bool blockGeneratingAllowed = true, bool blockConsumingAllowed = true)
			: m_blockGeneratingAllowed(blockGeneratingAllowed)
			, m_blockConsumingAllowed(blockConsumingAllowed)
		{}

	public:
		bool blockGeneratingAllowedAt(const Height&) override {
			return m_blockGeneratingAllowed;
		}

		bool blockConsumingAllowedAt(const Height&) override {
			return m_blockConsumingAllowed;
		}

	private:
		bool m_blockGeneratingAllowed;
		bool m_blockConsumingAllowed;
	};
}}
