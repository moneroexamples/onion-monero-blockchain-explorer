var config = {
    testnet: false, // this is adjusted page.h if needed. dont need to change manually
    stagenet: false, // this is adjusted page.h if needed. dont need to change manually
    coinUnitPlaces: 12,
    txMinConfirms: 10,         // corresponds to CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE in Monero
    txCoinbaseMinConfirms: 60, // corresponds to CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW in Monero
    coinSymbol: 'AEON',
    openAliasPrefix: "aeon",
    coinName: 'Aeon',
    coinUriPrefix: 'aeon:',
    addressPrefix: 0xB2,
    integratedAddressPrefix: 0x2733,
    subAddressPrefix: 0x06B8,
    addressPrefixTestnet: 0x0426,
    integratedAddressPrefixTestnet: 0x2C27,
    subAddressPrefixTestnet: 0x0AAC,
    addressPrefixStagenet: 0x011A,
    integratedAddressPrefixStagenet: 0x2C1B,
    subAddressPrefixStagenet: 0x0B20,
    feePerKB: new JSBigInt('2000000000'),//20^10 - for testnet its not used, as fee is dynamic.
    dustThreshold: new JSBigInt('1000000000'),//10^10 used for choosing outputs/change - we decompose all the way down if the receiver wants now regardless of threshold
    txChargeRatio: 0.5,
    defaultMixin: 4, // minimum mixin for hardfork v5
    txChargeAddress: '',
    idleTimeout: 30,
    idleWarningDuration: 20,
    maxBlockNumber: 500000000,
    avgBlockTime: 120,
    debugMode: false
};
