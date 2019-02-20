# properties for nemesis block compatible with catapult signature scheme used in tests
# for proper generation, update following config-network properties:
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
cat = true
cat.currency = true

[namespace>cat]

duration = 0

[mosaics]
cat:currency = true

[mosaic>cat:currency]
divisibility = 6
duration = 0
supply = {{xpx.supply}}

isTransferable = true
isSupplyMutable = false
isLevyMutable = false

[distribution>cat:currency]
{{#xpx.distribution}}
{{address}} = {{amount}}
{{/xpx.distribution}}
