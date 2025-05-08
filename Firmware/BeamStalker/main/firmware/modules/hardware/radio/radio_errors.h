#pragma once
#include <RadioLib.h>
#include <cstdio>
#include <cstdio>

static inline const char* radiolib_strerror(int16_t e)
{
    switch(e) {
    case RADIOLIB_ERR_NONE:                         return "OK";
    case RADIOLIB_ERR_UNKNOWN:                      return "UNKNOWN";
    case RADIOLIB_ERR_UNSUPPORTED:                  return "UNSUPPORTED";
    case RADIOLIB_ERR_CHIP_NOT_FOUND:               return "CHIP_NOT_FOUND";
    case RADIOLIB_ERR_INVALID_FREQUENCY:            return "INVALID_FREQUENCY";
    case RADIOLIB_ERR_INVALID_BIT_RATE:             return "INVALID_BIT_RATE";
    case RADIOLIB_ERR_INVALID_BANDWIDTH:            return "INVALID_BANDWIDTH";
    case RADIOLIB_ERR_INVALID_BIT_RATE_BW_RATIO:    return "INVALID_BR_BW_RATIO";
    case RADIOLIB_ERR_INVALID_SPREADING_FACTOR:     return "INVALID_SF";
    case RADIOLIB_ERR_INVALID_CODING_RATE:          return "INVALID_CR";
    case RADIOLIB_ERR_INVALID_OUTPUT_POWER:         return "INVALID_POWER";
    case RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH:      return "INVALID_PREAMBLE_LEN";
    case RADIOLIB_ERR_INVALID_SYNC_WORD:            return "INVALID_SYNC_WORD";
    case RADIOLIB_ERR_INVALID_NUM_SAMPLES:          return "INVALID_NUM_SAMPLES";
    case RADIOLIB_ERR_RX_TIMEOUT:                   return "RX_TIMEOUT";
    case RADIOLIB_ERR_TX_TIMEOUT:                   return "TX_TIMEOUT";
    case RADIOLIB_ERR_CRC_MISMATCH:                 return "CRC_ERROR";
    case RADIOLIB_ERR_PACKET_TOO_LONG:              return "PACKET_TOO_LONG";
    case RADIOLIB_ERR_SPI_CMD_TIMEOUT:              return "SPI_CMD_TIMEOUT";
    case RADIOLIB_ERR_SPI_CMD_INVALID:              return "SPI_CMD_INVALID";
    case RADIOLIB_ERR_SPI_WRITE_FAILED:             return "SPI_WRITE_FAILED";
    case RADIOLIB_ERR_INVALID_MODULATION:           return "INVALID_MODULATION";

    default:  {
        static char unknown[16];
        std::snprintf(unknown, sizeof(unknown), "ERR(%d)", e);
        return unknown;
    }
    }
}

#define RADIOLIB_CHECK(expr)                           \
    do {                                               \
        int16_t _st = (expr);                          \
        if(_st != RADIOLIB_ERR_NONE) {                 \
            printf("  ↳ %s → %s (%d)\n",               \
                   #expr, radiolib_strerror(_st), _st);\
            return _st;                                \
        }                                              \
    } while(0)
