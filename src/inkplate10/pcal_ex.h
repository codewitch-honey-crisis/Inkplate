#ifndef PCAL_EX_H
#define PCAL_EX_H
#include <stdbool.h>
#include <stdint.h>
typedef enum { INPUT, INPUT_PULLUP, INPUT_PULLDOWN, OUTPUT } pcal_ex_mode_t;

typedef enum { CHANGE,   FALLING,      RISING } pcal_ex_int_type_t;
typedef enum { INTPORTA, INTPORTB             } pcal_ex_int_port_t;
typedef enum { LOW,      HIGH                 } pcal_ex_level_t;

#ifdef __cplusplus
extern "C" {
#endif
bool pcal_ex_init(void);
bool pcal_ex_set_direction_ext(uint8_t pin, pcal_ex_mode_t mode);
bool pcal_ex_set_level_ext(uint8_t pin, pcal_ex_level_t level);
int pcal_ex_get_level_ext(uint8_t pin);
bool pcal_ex_set_int_pin_ext(uint8_t pin);
bool pcal_ex_clear_int_pin_ext(uint8_t pin);
int pcal_ex_get_int_ext(void);
bool pcal_ex_set_port_ext(uint16_t values);
int pcal_ex_get_port_ext(void);

bool pcal_ex_set_direction_int(uint8_t pin, pcal_ex_mode_t mode);
bool pcal_ex_set_level_int(uint8_t pin, pcal_ex_level_t level);
int pcal_ex_get_level_int(uint8_t pin);
bool pcal_ex_set_int_pin_int(uint8_t pin);
bool pcal_ex_clear_int_pin_int(uint8_t pin);
int pcal_ex_get_int_int(void);
bool pcal_ex_set_port_int(uint16_t values);
int pcal_ex_get_port_int(void);

#ifdef __cplusplus
}
#endif
#endif // PCAL_EX_H