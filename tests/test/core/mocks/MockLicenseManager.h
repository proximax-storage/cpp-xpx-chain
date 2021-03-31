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
		MockLicenseManager(bool blockHandlingAllowed = true)
			: m_blockHandlingAllowed(blockHandlingAllowed)
		{}

	public:
		bool blockAllowedAt(const Height&) override {
			return m_blockHandlingAllowed;
		}

		void setBlockElementSupplier(licensing::BlockElementSupplier) override {}

	private:
		bool m_blockHandlingAllowed;
	};
}}
