// LoRaWAN region - AU915 for Australia
#define LORAWAN_REGION          LORAMAC_REGION_AU915

// Device EUI - copy exactly from TTS (no spaces, no 0x prefix)
#define LORAWAN_DEVICE_EUI      "70B3D57ED00778CC"

// App EUI / Join EUI - use zeros if you set zeros in TTS
#define LORAWAN_APP_EUI         "0000000000000000"

// App Key - copy exactly from TTS (32 hex chars)
#define LORAWAN_APP_KEY         "001D144E7B2D8DB210166234FDD318AC"

// Channel mask - NULL lets the stack use AU915 FSB2 defaults
#define LORAWAN_CHANNEL_MASK    NULL
