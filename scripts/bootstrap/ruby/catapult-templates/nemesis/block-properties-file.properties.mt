# properties for nemesis block compatible with catapult signature scheme used in tests
# for proper generation, update following config-immutable properties:
# - shouldEnableVerifiableState = false
# - shouldEnableVerifiableReceipts = false

[nemesis]
networkIdentifier = {{network_identifier}}
nemesisGenerationHash = {{nemesis_generation_hash}}
nemesisSignerPrivateKey = {{nemesis_signer_private_key}}

[cpp]

cppFileHeader = ../HEADER.inc

[output]
cppFile =
binDirectory = ../seed/mijin-test

[namespaces]
prx = true
prx.xpx = true
prx.storage = true

[namespace>prx]

duration = 0

[mosaics]
prx:xpx = true
prx:storage = true

[mosaic>prx:xpx]
divisibility = 6
duration = 0
supply = {{xpx.supply}}

isTransferable = true
isSupplyMutable = false

[distribution>prx:xpx]
{{#xpx.distribution}}
{{address}} = {{amount}}
{{/xpx.distribution}}

[mosaic>prx:storage]
divisibility = 6
duration = 0
supply = {{xpx.supply}}

isTransferable = true
isSupplyMutable = true

[distribution>prx:storage]
{{#xpx.distribution}}
{{address}} = {{amount}}
{{/xpx.distribution}}
