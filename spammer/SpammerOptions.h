//
// Created by green on 20.08.18.
//

#ifndef CATAPULT_SERVER_SPAMMEROPTIONS_H
#define CATAPULT_SERVER_SPAMMEROPTIONS_H

#include <string>
#include <vector>

using namespace std;

class SpammerOptions {
public:
	string Host;
	unsigned short Port;
	int Rate;
	string Mode;
	int Total;
	string ApiNodePublicKey;
	string GenerationHash;
};

#endif //CATAPULT_SERVER_SPAMMEROPTIONS_H
