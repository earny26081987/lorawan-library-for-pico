/*!
 * \file      sx1262-board.h
 * \brief     SX1262 board hardware driver for Waveshare Pico-LoRa-SX1262
 */
#ifndef __SX1262_BOARD_H__
#define __SX1262_BOARD_H__

#include <stdint.h>
#include "sx126x.h"

void SX126xIoInit( void );
void SX126xIoIrqInit( DioIrqHandler dioIrq );
void SX126xIoDeInit( void );
void SX126xIoTcxoInit( void );
void SX126xIoRfSwitchInit( void );
void SX126xReset( void );
void SX126xWaitOnBusy( void );
void SX126xWakeup( void );
void SX126xWriteCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size );
uint8_t SX126xReadCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size );
void SX126xWriteRegisters( uint16_t address, uint8_t *buffer, uint16_t size );
void SX126xWriteRegister( uint16_t address, uint8_t value );
void SX126xReadRegisters( uint16_t address, uint8_t *buffer, uint16_t size );
uint8_t SX126xReadRegister( uint16_t address );
void SX126xWriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size );
void SX126xReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size );
void SX126xSetRfTxPower( int8_t power );
uint8_t SX126xGetDeviceId( void );
void SX126xAntSwOn( void );
void SX126xAntSwOff( void );
bool SX126xCheckRfFrequency( uint32_t frequency );
uint32_t SX126xGetDio1PinState( void );
uint32_t SX126xGetBoardTcxoWakeupTime( void );
RadioOperatingModes_t SX126xGetOperatingMode( void );
void SX126xWriteRegisters( uint16_t address, uint8_t *buffer, uint16_t size );
void SX126xReadRegisters( uint16_t address, uint8_t *buffer, uint16_t size );
void SX126xWriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size );
void SX126xReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size );
void SX126xSetOperatingMode( RadioOperatingModes_t mode );

#endif // __SX1262_BOARD_H__
