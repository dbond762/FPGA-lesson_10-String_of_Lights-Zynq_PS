#include "xparameters.h"
#include "xgpio.h"
#include "xil_printf.h"
#include <xil_types.h>
#include <xstatus.h>
#include "xiltimer.h"
#include "xinterrupt_wrap.h"

#include "rotate.h"

#define LED_BASEADDR   XPAR_AXI_GPIO_0_BASEADDR
#define BTN_BASEADDR   XPAR_AXI_GPIO_1_BASEADDR
#define SW_BASEADDR    XPAR_AXI_GPIO_2_BASEADDR
#define GPIO_CH        1U
#define GPIO_IR_MASK   XGPIO_IR_CH1_MASK

#define LED_WIDTH  4U
#define BTN_WIDTH  4U
#define SW_WIDTH   2U

#define LED_DIRECTION_MASK 0x0U                           // 4 виходи
#define BTN_DIRECTION_MASK ( ( 1U << BTN_WIDTH ) - 1U )   // 4 входи
#define SW_DIRECTION_MASK  ( ( 1U << SW_WIDTH  ) - 1U )   // 2 входи

#define TICK_MS         10U
#define LED_CHANGE_STEP 10U
#define LED_MIN_TICKS   10U
#define LED_MAX_TICKS   50U
#define DEBOUNCE_TICKS  3U

#define LED_DEFAULT       0x1U
#define LED_DEFAULT_TICKS 20U

#define DIR_SW_MASK     0x1
#define SLOWER_BTN_MASK 0x1
#define PAUSE_BTN_MASK  0x4
#define PLAY_BTN_MASK   0x2
#define FASTER_BTN_MASK 0x8

static volatile u32 tick          = 0U;
static volatile u32 btn_edge_tick = 0U;
static volatile u8  btn_pending   = FALSE;

static XGpio led_gpio, btn_gpio, sw_gpio;

static void TickHandler( void *CallBackRef, u32 StatusEvent )
{
    ( void )CallBackRef;
    ( void )StatusEvent;
    
    tick++;
}

static void GpioIsr( void *CallBackRef )
{
    XGpio *gpio = ( XGpio * )CallBackRef;
    
    btn_edge_tick = tick;
    btn_pending   = TRUE;
    XGpio_InterruptClear( gpio, GPIO_IR_MASK );
}

static XStatus SetupGpio( XGpio *gpio, UINTPTR base, u32 dir, u32 with_interrupt )
{
    XGpio_Config *cfg = XGpio_LookupConfig( base );
    if ( cfg == NULL )
    {
        return XST_FAILURE;
    }

    XStatus status = XGpio_CfgInitialize( gpio, cfg, cfg->BaseAddress );
    if ( status != XST_SUCCESS )
    {
        return status;
    }

    XGpio_SetDataDirection( gpio, GPIO_CH, dir );
    
    if ( with_interrupt )
    {
        XSetPriorityTriggerType( cfg->IntrId, XINTERRUPT_DEFAULT_PRIORITY, cfg->IntrParent );
        XConnectToInterruptCntrl( cfg->IntrId, GpioIsr, gpio, cfg->IntrParent );
        XEnableIntrId( cfg->IntrId, cfg->IntrParent );

        XGpio_InterruptEnable( gpio, GPIO_IR_MASK );
        XGpio_InterruptGlobalEnable( gpio );
    }

    return XST_SUCCESS;
}

XStatus init( void )
{
    XTimer_SetInterval( TICK_MS );
    XTimer_SetHandler( TickHandler, NULL, XINTERRUPT_DEFAULT_PRIORITY );

    XStatus status = XST_SUCCESS;
    
    status = SetupGpio( &led_gpio, LED_BASEADDR, LED_DIRECTION_MASK, FALSE );
    if ( status != XST_SUCCESS )
    {
        xil_printf( "LED Gpio Initialization Failed\r\n" );
        return status;
    }
    
    status = SetupGpio( &btn_gpio, BTN_BASEADDR, BTN_DIRECTION_MASK, TRUE );
    if ( status != XST_SUCCESS )
    {
        xil_printf( "BTN Gpio Initialization Failed\r\n" );
        return status;
    }
    
    status = SetupGpio( &sw_gpio, SW_BASEADDR, SW_DIRECTION_MASK, TRUE );
    if ( status != XST_SUCCESS )
    {
        xil_printf( "SW Gpio Initialization Failed\r\n" );
        return status;
    }

    return XST_SUCCESS;
}

int main( void )
{
    XStatus status = init();
    if ( status != XST_SUCCESS )
    {
        return status;
    }
    
    u32 led            = 0x1U;
    u32 last_led_tick  = 0U;
    u32 led_step_ticks = LED_DEFAULT_TICKS;
    u32 led_started    = TRUE;

    u32 btn       = 0x0U;
    u32 sw        = XGpio_DiscreteRead( &sw_gpio, GPIO_CH );
    u32 dir_right = sw & DIR_SW_MASK;

    while ( TRUE )
    {
        if ( led_started && ( tick - last_led_tick ) >= led_step_ticks )
        {
            last_led_tick += led_step_ticks;
            
            led = dir_right ? rot_r_4( led ) : rot_l_4( led );
            XGpio_DiscreteWrite( &led_gpio, GPIO_CH, led );
        }

        if ( btn_pending && ( tick - btn_edge_tick ) >= DEBOUNCE_TICKS )
        {
            btn_pending = FALSE;
            
            sw = XGpio_DiscreteRead( &sw_gpio, GPIO_CH );
            dir_right = sw & DIR_SW_MASK;

            btn = XGpio_DiscreteRead( &btn_gpio, GPIO_CH );
            if ( btn & SLOWER_BTN_MASK )
            {
                if ( led_step_ticks > LED_MIN_TICKS )
                {
                    led_step_ticks -= LED_CHANGE_STEP;
                }
            }
            else if ( btn & PAUSE_BTN_MASK )
            {
                led_started = FALSE;
            }
            else if ( btn & PLAY_BTN_MASK )
            {
                led_started   = TRUE;
                last_led_tick = tick;
            }
            else if ( btn & FASTER_BTN_MASK )
            {
                if ( led_step_ticks < LED_MAX_TICKS )
                {
                    led_step_ticks += LED_CHANGE_STEP;
                }
            }
            else
            {
            }
        }

        __asm__ volatile ( "wfi" );  // спати до наступного переривання (тік/кнопка)
    }

    return XST_SUCCESS;
}
