/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NetworkConfigTestUtils.h"

namespace catapult { namespace test {

	std::string networkConfig() {
		return
			"[network]\n"
			"\n"
			"publicKey = B4F12E7C9F6946091E2CB8B6D3A12B50D17CCBBF646386EA27CE2946A7423DCF\n"
			"\n"
			"[chain]\n"
			"\n"
			"blockGenerationTargetTime = 15s\n"
			"blockTimeSmoothingFactor = 3000\n"
			"\n"
			"greedDelta = 0.5\n"
			"greedExponent = 2\n"
			"\n"
			"importanceGrouping = 7\n"
			"maxRollbackBlocks = 360\n"
			"maxDifficultyBlocks = 3\n"
			"\n"
			"maxTransactionLifetime = 24h\n"
			"maxBlockFutureTime = 10s\n"
			"\n"
			"maxMosaicAtomicUnits = 9'000'000'000'000'000\n"
			"\n"
			"totalChainImportance = 8'999'999'998'000'000\n"
			"minHarvesterBalance = 1'000'000'000'000\n"
			"harvestBeneficiaryPercentage = 10\n"
			"\n"
			"blockPruneInterval = 360\n"
			"maxTransactionsPerBlock = 200'000\n\n";
	}

	std::string supportedVersions() {
		return
			"{\n"
			"\t\"entities\": [\n"
			"\t\t{\n"
			"\t\t\t\"name\": \"Block\",\n"
			"\t\t\t\"type\": \"33091\",\n"
			"\t\t\t\"supportedVersions\": [3]\n"
			"\t\t}"
			"\t]\n"
			"}";
	}
}}


