/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/NetworkInfo.h"
#include "catapult/utils/ConfigurationBag.h"
#include "catapult/types.h"

namespace catapult { namespace config {

	/// Immutable network configuration settings.
	struct ImmutableConfiguration {
	public:
		/// Network identifier.
		model::NetworkIdentifier NetworkIdentifier;

		/// Nemesis generation hash.
		catapult::GenerationHash GenerationHash;

		/// \c true if block chain should calculate state hashes so that state is fully verifiable at each block.
		bool ShouldEnableVerifiableState;

		/// \c true if block chain should calculate receipts so that state changes are fully verifiable at each block.
		bool ShouldEnableVerifiableReceipts;

		/// Mosaic id used as primary chain currency.
		MosaicId CurrencyMosaicId;

		/// Mosaic id used to provide harvesting ability.
		MosaicId HarvestingMosaicId;

		/// Mosaic id used to provide storage ability.
		MosaicId StorageMosaicId;

		/// Mosaic id used to provide streaming ability.
		MosaicId StreamingMosaicId;

		/// Mosaic id used to provide review ability.
		MosaicId ReviewMosaicId;

		/// Mosaic id for super contracts.
		MosaicId SuperContractMosaicId;

		/// XAR mosaic id.
		MosaicId XarMosaicId;

		/// Initial currency atomic units available in the network.
		Amount InitialCurrencyAtomicUnits;

	private:
		ImmutableConfiguration() = default;

	public:
		/// Creates an uninitialized immutable network configuration.
		static ImmutableConfiguration Uninitialized();

		/// Loads an immutable network configuration from \a bag.
		static ImmutableConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};

	/// Gets unresolved currency mosaic id from \a config.
	UnresolvedMosaicId GetUnresolvedCurrencyMosaicId(const ImmutableConfiguration& config);

	/// Gets unresolved storage mosaic id from \a config.
	UnresolvedMosaicId GetUnresolvedStorageMosaicId(const ImmutableConfiguration& config);

	/// Gets unresolved streaming mosaic id from \a config.
	UnresolvedMosaicId GetUnresolvedStreamingMosaicId(const ImmutableConfiguration& config);

	/// Gets unresolved review mosaic id from \a config.
	UnresolvedMosaicId GetUnresolvedReviewMosaicId(const ImmutableConfiguration& config);

	/// Gets unresolved super contract mosaic id from \a config.
	UnresolvedMosaicId GetUnresolvedSuperContractMosaicId(const ImmutableConfiguration& config);

	/// Gets unresolved XAR mosaic id from \a config.
	UnresolvedMosaicId GetUnresolvedXarMosaicId(const ImmutableConfiguration& config);
}}
