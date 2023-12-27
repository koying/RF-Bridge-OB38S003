//=========================================================
//=========================================================

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
//#include <stdint.h>
//#include <stdlib.h>

// printf() requires a decent amount of code space and ram which we would like to avoid
// and printf is not particularly once tasmota packet format is used
#include <stdio.h>

// borrowed from area-8051 uni-stc HAL...
#include "delay.h"

#include "hal.h"

// FIXME: probably avoid this dependency later
#include "ob38s003_sfr.h"
#include "rcswitch.h"
#include "timer.h"
#include "uart.h"
#include "uart_software.h"

// constants


// FIXME: we instead define a similar macro in delay.h
//#define SYSCLK 16000000


// this is returned along with an ack to a specific serial command
// good demonstration that serial pins and web interface in tasmota is working
#define FIRMWARE_VERSION 0x03


// DEBUG: bench sensor
//21:47:05.533 RSL: RESULT = {"Time":"2023-08-05T21:47:05","RfReceived":{"Sync":10280,"Low":340,"High":1010,"Data":"80650A","RfKey":"None"}}
//19:16:47.313 RSL: RESULT = {"Time":"2023-08-05T19:16:47","RfReceived":{"Sync":10280,"Low":340,"High":1010,"Data":"80650E","RfKey":"None"}}


// FIXME: these defines do not do anything yet
// this decodes radio packets on the microcontroller
//#define RCSWITCH_MODE 0

// this simply monitors signal levels in/out from radio receiver/transmitter chips and mirrors the levels on the TXD/RXD pins going to ESP8265
// radio packet decoding is then the responsibility of the ESP8265
//#define MIRROR_MODE 1

const bool gMirrorMode = true;

// sdccman sec. 3.8.1 indicates isr prototype must appear or be included in the file containing main
// millisecond tick count
//extern void timer0_isr(void) __interrupt (1);
// software uart
extern void tm0(void)        __interrupt (1);
extern void timer1_isr(void) __interrupt (3);
extern void uart_isr(void)   __interrupt (4);
extern void timer2_isr(void) __interrupt (5);


//-----------------------------------------------------------------------------
// FIXME: this is sometimes needed to initialize external ram, setup watch dog timer, etc.
//-----------------------------------------------------------------------------
void __sdcc_external_startup(void)
{

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void uart_state_machine(const unsigned int rxdata)
{
    // FIXME: need to check what appropriate initialization values are
    static UART_STATE_T state = IDLE;
    UART_COMMAND_T command = NONE;
    
    uint16_t bucket = 0;

    // FIXME: need to know what initialization value is appropriate
    uint8_t position = 0;
    
    // track count of entries into function
    static uint16_t idleResetCount = 0;

    // FIXME: this seems to reset state machine if we do not receive data after some time
    if (rxdata == UART_NO_DATA)
    {
        if (state == IDLE)
            idleResetCount = 0;
        else
        {
            idleResetCount++;
            
            // FIXME: no magic numbers
            if (idleResetCount > 30000)
            {
                idleResetCount = 0;
                
                state = IDLE;
                command = NONE;
            }
        }
    }
    else
    {
        idleResetCount = 0;
        
    }
    
    // state machine for UART
    switch(state)
    {
        // check if UART_SYNC_INIT got received
        case IDLE:
            if ((rxdata & 0xFF) == RF_CODE_START)
            {
                state = SYNC_INIT;
            }
            
            break;

        // sync byte got received, read command
        case SYNC_INIT:
            command = rxdata & 0xFF;
            
            // FIXME: not sure if setting this here is correct
            state = SYNC_FINISH;

            // check if some data needs to be received
            switch(command)
            {
                case RF_CODE_LEARN:
                    break;
                // do original sniffing
                case RF_CODE_RFIN:
                    break;
                case RF_CODE_RFOUT:
                    break;
                case RF_DO_BEEP:
                    // FIXME: replace with timer rather than delay(), although appears original code was blocking too
                    buzzer_on();
                    delay1ms(50);
                    buzzer_off();

                    // send acknowledge
                    uart_put_command(RF_CODE_ACK);
                    break;
                case RF_ALTERNATIVE_FIRMWARE:
                    uart_put_command(RF_CODE_ACK);
                    uart_put_command(FIRMWARE_VERSION);
                    break;
                case RF_CODE_SNIFFING_ON:
                    //gSniffingMode = ADVANCED;
                    //PCA0_DoSniffing(RF_CODE_SNIFFING_ON);
                    //gLastSniffingCommand = RF_CODE_SNIFFING_ON;
                    break;
                case RF_CODE_SNIFFING_OFF:
                    // set desired RF protocol PT2260
                    //gSniffingMode = STANDARD;
                    // re-enable default RF_CODE_RFIN sniffing
                    //pca_start_sniffing(RF_CODE_RFIN);
                    //gLastSniffingCommand = RF_CODE_RFIN;
                    break;
                case RF_CODE_ACK:
                    // re-enable default RF_CODE_RFIN sniffing
                    //gLastSniffingCommand = PCA0_DoSniffing(gLastSniffingCommand);
                    //state = IDLE;
                    break;
                case SINGLE_STEP_DEBUG:
                    //gSingleStep = true;
                    break;

                    
                // wait until data got transfered
                case RF_FINISHED:
                    //if (trRepeats == 0)
                    //{
                    //    // disable RF transmit
                    //    tdata_off();
                    //
                    //    uart_put_command(RF_CODE_ACK);
                    //} else {
                    //    gRFState = RF_IDLE;
                    //}
                    break;

                // unknown command
                default:
                    state = IDLE;
                    command = NONE;
                    break;
            }
            break;

        // Receiving UART data length
        case RECEIVE_LEN:
            //position = 0;
            //gLength = rxdata & 0xFF;
            //if (gLength > 0)
            //{
            //    // stop sniffing while handling received data
            //    pca_stop_sniffing();
            //    state = RECEIVING;
            //} else {
            //    state = SYNC_FINISH;
            //}
            
            break;

        // receiving UART data
        case RECEIVING:
            //gRFData[position] = rxdata & 0xFF;
            //position++;

            //if (position == gLength)
            //{
            //    state = SYNC_FINISH;
            //}
            //else if (position >= RF_DATA_BUFFERSIZE)
            //{
            //    gLength = RF_DATA_BUFFERSIZE;
            //    state = SYNC_FINISH;
            //}
            break;

        // wait and check for UART_SYNC_END
        case SYNC_FINISH:
            if ((rxdata & 0xFF) == RF_CODE_STOP)
            {
                state = IDLE;
            }
            break;
    }
}



// FIXME: some of these function names really need fixing
void radio_decode_report(void)
{
    uint8_t i = 0;
    uint8_t b = 0;

    // packet start sequence
    putchar(RF_CODE_START);
    putchar(RF_CODE_RFIN);
    
    // sync, low, high timings
    putchar((timings[0] >> 8) & 0xFF);
    putchar(timings[0] & 0xFF);

    
    // FIXME: not sure if we should compute an average or something
    // FIXME: handle inverted signal?
    putchar((timings[2] >> 8) & 0xFF);
    putchar( timings[2] & 0xFF);
    putchar((timings[1] >> 8) & 0xFF);
    putchar( timings[1] & 0xFF);
    
    // data
    // FIXME: strange that shifting by ZERO works but omitting the shift does not
    putchar((get_received_value() >> 16) & 0xFF);
    putchar((get_received_value() >>  8) & 0xFF);
    putchar((get_received_value() >>  0) & 0xFF);
    
    // packet stop
    putchar(RF_CODE_STOP);
}

// FIXME: think Tasmota ignores this for now because command is unknown?
void radio_timings(void)
{
    unsigned int index;
    
    for (index = 0; index < get_received_bitlength() * 2; index++)
    {
        // packet start sequence
        putc(RF_CODE_START);
        putc(0xAF);
        
        // sync, low, high timings
        putc((timings[index] >> 8) & 0xFF);
        putc(timings[index] & 0xFF);
    }
    
    putc(RF_CODE_STOP);
}

#if 0
    // we avoid use of printf but may be able to adapt this to wifi serial protocol format?
    void radio_decode_debug(void)
    {
        printf_fast("Received: ");
        printf_fast("0x%lx", get_received_value());
        printf_fast(" / ");
        printf_fast("%u", get_received_bitlength() );
        printf_fast("bit ");
        printf_fast("Protocol: ");
        printf_fast("%u", get_received_protocol() );
        
        printf_fast("\r\n");
    }
#endif

#if 0
    void startup_debug(const __idata unsigned char* stackStart)
    {
        // just demonstrate serial uart is working basically
        printf_fast("Startup...\r\n");
        
        printf_fast("Start of stack: %p\r\n", stackStart);
        
        //printf_fast("num. of protocols: %u\r\n", numProto);

        // DEBUG: demonstrates that we cannot write above SP (stack pointer)
        //*gStackStart       = 0x5a;
        //*(gStackStart + 1) = 0x5a;
        //printf_fast("gStackStart[%p]: 0x%02x\r\n", gStackStart,   *gStackStart);
        //printf_fast("gStackStart[%p]: 0x%02x\r\n", gStackStart+1, *(gStackStart + 1));
    }
#endif

void startup_beep(void)
{
    // FIXME: startup beep helpful or annoying?
    buzzer_on();
    delay1ms(20);
    buzzer_off();   
}

void startup_blink(void)
{
    // startup blink
    led_on();
    delay1ms(500);
    led_off();
}

void startup_reset_status(void)
{
    uint8_t index;
    
    for (index = 1; index <= RSTS; index++)
    {
        led_on();
        //reset_pin_on();
        delay1ms(1000);
        led_off();
        //reset_pin_off();
        delay1ms(1000);
    }
}


//-----------------------------------------------------------------------------
// main() Routine
// ----------------------------------------------------------------------------
int main (void)
{

    // holdover from when we considered using rtos
    //const __idata unsigned char* stackStart = (__idata unsigned char*) get_stack_pointer() + 1;

    // FIXME: other protocols are not working
    const uint8_t repeats = 8;
    const uint8_t protocolId = 1;
    
    // track elapsed time for doing something periodically (e.g., every 10 seconds)
    unsigned long previousTimeSendRadio = 0;
    unsigned long previousTimeHeartbeat = 0;
    unsigned long elapsedTimeSendRadio;
    unsigned long elapsedTimeHeartbeat;
    unsigned long heartbeat = 0;
    
    
    // upper eight bits hold error or no data flags
    unsigned int rxdata = UART_NO_DATA;
    
    // look for level transitions on pins rather than absolute level, so that we can count signal edges
    bool        rdataLevelOld;
    static bool rdataLevelNew = false;
    
    bool        tdataLevelOld;
    static bool tdataLevelNew = false;
    

    // hardware initialization
    set_clock_1t_mode();
    init_port_pins();
    
    // set default pin levels
    led_off();
    buzzer_off();
    tdata_off();
    
    // FIXME: would like to read a pin somewhere to choose mode?
    //        and allow switching between modes?
    // choose at startup if we want to mirror radio pins to esp8265 or enable uart to output decoded radio packet instead
    if (!gMirrorMode)
    {
        // setup hardware serial
        init_uart();
        uart_rx_enabled();
        
        // hardware serial interrupt
        init_serial_interrupt();
    }
    
    // FIXME: pin used for now to read for determining mirror mode or decoding mode
    // default state is reset high if using software uart
    //reset_pin_on();
    
    // software serial
    //init_software_uart();
    //enable_timer0();
        
    // provides one millisecond tick
    // (warning: cannot be used at the same time as software uart for now)
    //init_timer0();
        
    // provides ten microsecond tick
    init_timer1();
    
    // tick style functionality
    enable_timer1_interrupt();
    
    // timer supports compare and capture module for determining pulse lengths of received radio signals
    init_timer2_capture();

    // radio receiver edge detection
    init_capture_interrupt();
    
    
    // enable radio receiver
    radio_receiver_on();
    
    //startup_beep();
    //startup_debug(stackStart);
    //startup_blink();
    startup_reset_status();
    
    // just to give some startup time
    delay1ms(500);

    // enable interrupts
    enable_global_interrupts();

        
    // watchdog will force a reset, unless we periodically write to it, demonstrating loop is not stuck somewhere
    enable_watchdog();

    while (true)
    {

        // if this is not periodically called, watchdog will force microcontroller reset
        refresh_watchdog();
    
    
        // mirror radio voltage levels to esp8265, otherwise output decoded serial data
        if (gMirrorMode)
        {
            // mirror radio pin levels to uart pins (used as gpio)
            // so that esp8265 can effectively decode the same signals
            rdataLevelOld = rdataLevelNew;
            rdataLevelNew = rdata_level();
            
            // look for a level change (i.e., an edge)
            // because I am thinking expliciting setting pin output constantly, even with no change, is somehow a bad idea
            if (rdataLevelOld != rdataLevelNew)
            {
                // radio receiver
                if (rdataLevelNew)
                {
                    uart_tx_pin_on();
                } else {
                    uart_tx_pin_off();
                }
            }

            // FIXME: variable name may need to be changed
            //        (is actually uart rx pin level)
            tdataLevelOld = tdataLevelNew;
            tdataLevelNew = uart_rx_pin_level();
            
            if (tdataLevelOld != tdataLevelNew)
            {
                
                // radio transmitter
                if (tdataLevelNew)
                {
                    tdata_on();
                } else {
                    tdata_off();
                }
            }
        } else {

            // try to get one byte from uart rx buffer
            rxdata = uart_getc();

         
            // check if serial transmit buffer is empty
            if(!is_uart_tx_buffer_empty())
            {
                if (is_uart_tx_finished())
                {
                    // if not empty, set transmit interrupt flag, which triggers actual transmission
                    uart_init_tx_polling();
                }
            }
            

            // process serial receive data
            if (rxdata != UART_NO_DATA)
            {
                uart_state_machine(rxdata);
            }
            
        }


#if 1

        // when in mirror mode, just use successful radio decode (onboard microcontroller)
        // as sign to toggle led for human feedback
        //
        // otherwise send out decoded packet over serial port
        if (available() && gMirrorMode)
        {
            // flickers as packets detected so nice feedback to human
            led_toggle();
            
            // clears flag indicating packet was decoded
            reset_available();
        } else if (available() && !gMirrorMode)
        {
            // FIXME: there must be a better way to lock
            // this is needed to avoid corrupting the currently received packet
            disable_capture_interrupt();
            
            // formatted for tasmota
            radio_decode_report();
            
            // DEBUG: formatted like rc-switch example
            //radio_decode_debug();
            
            // DEBUG:
            //radio_timings();
            
            led_toggle();

            reset_available();
            
            enable_capture_interrupt();
            
            // DEBUG: using software uart
            // FIXME: a little dangerous as-is because basically sits in a while() loop ?
            // protocol index
            putc('p');
            putc('x');
            puthex2(get_received_protocol());
            putc(' ');

            // bits received
            putc('b');
            putc('x');
            puthex2(get_received_bitlength());
            putc('\r');
            putc('\n');
        }
    
#endif
        
        
#if 1
        // do something about every ten seconds to show loop is alive
        elapsedTimeHeartbeat = get_elapsed_timer1(previousTimeHeartbeat);

        if (elapsedTimeHeartbeat >= 1000000)
        {
            // test software uart
            //puthex2(heartbeat);
            //putc('\r');
            //putc('\n');
            
            led_toggle();
            
            previousTimeHeartbeat = get_current_timer1();
            
            heartbeat++;
        }
        
#endif
        
     
     
#if 0
        // FIXME: future use for transmitting
        // FIXME: should we check to see if we are in the middle of receiving?
        
        // periodically send out a radio transmission
        elapsedTimeSendRadio = get_elapsed_timer1(previousTimeSendRadio);

        if (elapsedTimeSendRadio >= 30000)
        {
            // FIXME: not sure if we NEED to disable radio receiver but we probably should (to avoid loopback)
            //radio_receiver_off();
            
            led_toggle();
            
            // FIXME: do stuff
            
            //radio_receiver_on();
            
            previousTimeSendRadio = get_current_timer1();
        }
        
#endif 
      
        
        

    }
    
}
