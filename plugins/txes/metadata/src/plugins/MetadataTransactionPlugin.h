/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/SupportedVersionsSupplier.h"
#include "catapult/plugins.h"
#include <memory>

namespace catapult { namespace model { class TransactionPlugin; } }

namespace catapult { namespace plugins {

	/// Creates an address metadata transaction plugin given \a supportedVersionsSupplier.
	PLUGIN_API
	std::unique_ptr<model::TransactionPlugin> CreateAddressMetadataTransactionPlugin(model::SupportedVersionsSupplier supportedVersionsSupplier);

	/// Creates a mosaic metadata transaction plugin given \a supportedVersionsSupplier.
	PLUGIN_API
	std::unique_ptr<model::TransactionPlugin> CreateMosaicMetadataTransactionPlugin(model::SupportedVersionsSupplier supportedVersionsSupplier);

	/// Creates a namespace metadata transaction plugin given \a supportedVersionsSupplier.
	PLUGIN_API
	std::unique_ptr<model::TransactionPlugin> CreateNamespaceMetadataTransactionPlugin(model::SupportedVersionsSupplier supportedVersionsSupplier);
}}
