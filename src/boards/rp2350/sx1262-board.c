/*!
 * \file      sx1262-board.c
 * \brief     Waveshare Pico-LoRa-SX1262 board driver for RP2350
 *
 * Pin mapping:
 *   NSS   -> GP3   SCK  -> GP10
 *   MOSI  -> GP11  MISO -> GP12
 *   RESET -> GP15  BUSY -> GP2
 *   DIO1  -> GP20
 */

#include <stdint.h>
#include <stdbool.h>
#include "hardware/gpio.h"
#include "board.h"
#include "pico/sync.h"
#include "gpio.h"
#include "spi.h"
#include "delay.h"
#include "sx126x-board.h"

static RadioOperatingModes_t OperatingMode;

void SX126xIoInit( void )
{
    // NSS - pre-load HIGH before enabling output so the SX1262 is never
    // accidentally selected during the RP2350 output-enable transition.
    gpio_init( SX126x.Spi.Nss.pin );
    gpio_put( SX126x.Spi.Nss.pin, 1 );
    gpio_set_dir( SX126x.Spi.Nss.pin, GPIO_OUT );

    // RESET - pre-load HIGH (deasserted) before enabling output.
    // Initialising here clears the RP2350 pad-isolation (ISO) bit once,
    // before SX126xReset() drives it low, avoiding any ISO-clear glitch
    // inside the reset sequence itself.
    gpio_init( SX126x.Reset.pin );
    gpio_put( SX126x.Reset.pin, 1 );
    gpio_set_dir( SX126x.Reset.pin, GPIO_OUT );

    // BUSY - input, no pull (push-pull output from SX1262)
    gpio_init( SX126x.BUSY.pin );
    gpio_set_dir( SX126x.BUSY.pin, GPIO_IN );
    gpio_disable_pulls( SX126x.BUSY.pin );

    // DIO1 - input, no pull
    gpio_init( SX126x.DIO1.pin );
    gpio_set_dir( SX126x.DIO1.pin, GPIO_IN );
    gpio_disable_pulls( SX126x.DIO1.pin );
}

void SX126xIoIrqInit( DioIrqHandler dioIrq )
{
    gpio_set_irq_enabled_with_callback( SX126x.DIO1.pin, GPIO_IRQ_EDGE_RISE, true,
                                        (gpio_irq_callback_t)dioIrq );
}

void SX126xIoDeInit( void )
{
    GpioInit( &SX126x.Spi.Nss, SX126x.Spi.Nss.pin, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );
    GpioInit( &SX126x.BUSY,    SX126x.BUSY.pin,    PIN_INPUT,  PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &SX126x.DIO1,    SX126x.DIO1.pin,    PIN_INPUT,  PIN_PUSH_PULL, PIN_NO_PULL, 0 );
}

void SX126xIoTcxoInit( void ) { }

void SX126xIoDbgInit( void ) { }

void SX126xIoRfSwitchInit( void )
{
    // GP22 controls the on-board RF switch; drive high to enable TX/RX path.
    gpio_init( 22 );
    gpio_put( 22, 1 );
    gpio_set_dir( 22, GPIO_OUT );
}

uint32_t SX126xGetBoardTcxoWakeupTime( void ) { return 0; }

RadioOperatingModes_t SX126xGetOperatingMode( void ) { return OperatingMode; }

void SX126xSetOperatingMode( RadioOperatingModes_t mode ) { OperatingMode = mode; }

void SX126xReset( void )
{
    // NSS must be deasserted during the reset pulse.
    gpio_put( SX126x.Spi.Nss.pin, 1 );

    // Assert RESET: pre-load latch to 0, then enable output.
    // The pin may be in INPUT mode from the tail of a previous reset call,
    // so we set the latch value first to avoid driving HIGH momentarily.
    gpio_put( SX126x.Reset.pin, 0 );
    gpio_set_dir( SX126x.Reset.pin, GPIO_OUT );
    DelayMs( 20 );

    // Release RESET.
    // CRITICAL for RP2350: enable the internal pull-up BEFORE switching to
    // input mode. On RP2040 the SX1262's 50 kOhm NRESET pull-up holds the
    // pad HIGH across the brief gap between gpio_set_dir(IN) and
    // gpio_pull_up(). On RP2350 the pad discharges faster (smaller parasitic
    // capacitance on the newer process node), so that gap lets NRESET droop
    // below the logic-low threshold and the chip sees a second reset pulse,
    // keeping BUSY permanently asserted.
    gpio_pull_up( SX126x.Reset.pin );
    gpio_set_dir( SX126x.Reset.pin, GPIO_IN );
    DelayMs( 10 );

    // SX1262 holds BUSY high during post-reset RC calibration (~3.5 ms).
    SX126xWaitOnBusy( );
}

void SX126xWaitOnBusy( void )
{
    while( GpioRead( &SX126x.BUSY ) == 1 );
}

void SX126xWakeup( void )
{
    uint32_t irq_state = save_and_disable_interrupts( );
    GpioWrite( &SX126x.Spi.Nss, 0 );
    SpiInOut( &SX126x.Spi, RADIO_GET_STATUS );
    SpiInOut( &SX126x.Spi, 0x00 );
    GpioWrite( &SX126x.Spi.Nss, 1 );
    SX126xWaitOnBusy( );
    restore_interrupts( irq_state );
}

void SX126xWriteCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
    SX126xCheckDeviceReady( );
    GpioWrite( &SX126x.Spi.Nss, 0 );
    SpiInOut( &SX126x.Spi, ( uint8_t )command );
    for( uint16_t i = 0; i < size; i++ )
    {
        SpiInOut( &SX126x.Spi, buffer[i] );
    }
    GpioWrite( &SX126x.Spi.Nss, 1 );
    if( command != RADIO_SET_SLEEP )
    {
        SX126xWaitOnBusy( );
    }
}

uint8_t SX126xReadCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
    uint8_t status = 0;
    SX126xCheckDeviceReady( );
    GpioWrite( &SX126x.Spi.Nss, 0 );
    SpiInOut( &SX126x.Spi, ( uint8_t )command );
    status = SpiInOut( &SX126x.Spi, 0x00 );
    for( uint16_t i = 0; i < size; i++ )
    {
        buffer[i] = SpiInOut( &SX126x.Spi, 0x00 );
    }
    GpioWrite( &SX126x.Spi.Nss, 1 );
    SX126xWaitOnBusy( );
    return status;
}

void SX126xWriteRegister( uint16_t address, uint8_t value )
{
    SX126xWriteRegisters( address, &value, 1 );
}

uint8_t SX126xReadRegister( uint16_t address )
{
    uint8_t data;
    SX126xReadRegisters( address, &data, 1 );
    return data;
}

void SX126xSetRfTxPower( int8_t power )
{
    if( power > 22 )  power = 22;
    if( power < -3 )  power = -3;
    SX126xSetPaConfig( 0x04, 0x07, 0x00, 0x01 );
    SX126xSetTxParams( power, RADIO_RAMP_40_US );
}

uint8_t SX126xGetDeviceId( void ) { return SX1262; }

void SX126xAntSwOn( void ) { }
void SX126xAntSwOff( void ) { }

bool SX126xCheckRfFrequency( uint32_t frequency ) { return true; }

uint32_t SX126xGetDio1PinState( void )
{
    return GpioRead( &SX126x.DIO1 );
}

void SX126xWriteRegisters( uint16_t address, uint8_t *buffer, uint16_t size )
{
    SX126xCheckDeviceReady( );
    GpioWrite( &SX126x.Spi.Nss, 0 );
    SpiInOut( &SX126x.Spi, RADIO_WRITE_REGISTER );
    SpiInOut( &SX126x.Spi, ( address & 0xFF00 ) >> 8 );
    SpiInOut( &SX126x.Spi, address & 0x00FF );
    for( uint16_t i = 0; i < size; i++ )
    {
        SpiInOut( &SX126x.Spi, buffer[i] );
    }
    GpioWrite( &SX126x.Spi.Nss, 1 );
    SX126xWaitOnBusy( );
}

void SX126xReadRegisters( uint16_t address, uint8_t *buffer, uint16_t size )
{
    SX126xCheckDeviceReady( );
    GpioWrite( &SX126x.Spi.Nss, 0 );
    SpiInOut( &SX126x.Spi, RADIO_READ_REGISTER );
    SpiInOut( &SX126x.Spi, ( address & 0xFF00 ) >> 8 );
    SpiInOut( &SX126x.Spi, address & 0x00FF );
    SpiInOut( &SX126x.Spi, 0x00 );
    for( uint16_t i = 0; i < size; i++ )
    {
        buffer[i] = SpiInOut( &SX126x.Spi, 0x00 );
    }
    GpioWrite( &SX126x.Spi.Nss, 1 );
    SX126xWaitOnBusy( );
}

void SX126xWriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
    SX126xCheckDeviceReady( );
    GpioWrite( &SX126x.Spi.Nss, 0 );
    SpiInOut( &SX126x.Spi, RADIO_WRITE_BUFFER );
    SpiInOut( &SX126x.Spi, offset );
    for( uint16_t i = 0; i < size; i++ )
    {
        SpiInOut( &SX126x.Spi, buffer[i] );
    }
    GpioWrite( &SX126x.Spi.Nss, 1 );
    SX126xWaitOnBusy( );
}

void SX126xReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
    SX126xCheckDeviceReady( );
    GpioWrite( &SX126x.Spi.Nss, 0 );
    SpiInOut( &SX126x.Spi, RADIO_READ_BUFFER );
    SpiInOut( &SX126x.Spi, offset );
    SpiInOut( &SX126x.Spi, 0x00 );
    for( uint16_t i = 0; i < size; i++ )
    {
        buffer[i] = SpiInOut( &SX126x.Spi, 0x00 );
    }
    GpioWrite( &SX126x.Spi.Nss, 1 );
    SX126xWaitOnBusy( );
}
