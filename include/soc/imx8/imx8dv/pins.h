/*==========================================================================*/
/*!
 * @file config/mx8dv/pins.h
 *
 * Header file used to configure SoC pin list. 
 *
 */
/*==========================================================================*/

#ifndef _SC_PINS_H
#define _SC_PINS_H

#define SC_PIN_W            1

/*!
 * This type is used to indicate a pin.
 */
typedef enum sc_pin_e
{
    SC_P_PWM0_OUT,
    SC_P_PWM1_OUT
} sc_pin_t;

#define SC_P_LAST           SC_P_PWM1_OUT

#ifdef DEBUG
    #define PNAME_INIT      \
        "PWM0_OUT",         \
        "PWM1_OUT",
#endif

#endif /* _SC_PINS_H */

