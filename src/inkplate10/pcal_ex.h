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
/// @brief Initialize the I/O expander
/// @return True if success, otherwise false
bool pcal_ex_init(void);
/// @brief Sets the direction for an external pin
/// @param pin The pin to set
/// @param mode The mode to set
/// @return True if successful, otherwise false
bool pcal_ex_set_direction_ext(uint8_t pin, pcal_ex_mode_t mode);
/// @brief Sets the level for an external pin
/// @param pin The pin
/// @param level The level
/// @return True if successful, otherwise false
bool pcal_ex_set_level_ext(uint8_t pin, pcal_ex_level_t level);
/// @brief Gets the level of an external pin
/// @param pin The pin
/// @return The level, or -1 on error
int pcal_ex_get_level_ext(uint8_t pin);
/// @brief Sets the interrupt for an external pin
/// @param pin The pin
/// @return True on success, otherwise false
bool pcal_ex_set_int_pin_ext(uint8_t pin);
/// @brief Clears the interrupt for an external pin
/// @param pin The pin
/// @return True on success, otherwise false
bool pcal_ex_clear_int_pin_ext(uint8_t pin);
/// @brief Gets the interrupt for an external pin
/// @return The interrupt pin or -1 on error
int pcal_ex_get_int_ext(void);
/// @brief Sets the port for external pins
/// @param values the bitmask of values
/// @return True on success, otherwise false
bool pcal_ex_set_port_ext(uint16_t values);
/// @brief Gets the port for external pins
/// @return a bitmask of values or -1 on error
int pcal_ex_get_port_ext(void);

/* WARNING WARNING WARNING WARNING

Using the following functions with pin values of < 9 may damage your Inkplate.

*/
/// @brief Sets the direction for an internal pin
/// @param pin The pin (<9 may damage the inkplate)
/// @param mode The mode to set
/// @return True on success, otherwise false
bool pcal_ex_set_direction_int(uint8_t pin, pcal_ex_mode_t mode);
/// @brief Sets the direction for an internal pin
/// @param pin The pin (<9 may damage the inkplate)
/// @param level The level to set
/// @return True on success, otherwise false
bool pcal_ex_set_level_int(uint8_t pin, pcal_ex_level_t level);
/// @brief Gets the level for an internal pin
/// @param pin The pin (<9 may damage the inkplate)
/// @return The level on success, otherwise -1
int pcal_ex_get_level_int(uint8_t pin);
/// @brief Sets the interrupt an internal pin
/// @param pin The pin (<9 may damage the inkplate)
/// @return True on success, otherwise false
bool pcal_ex_set_int_pin_int(uint8_t pin);
/// @brief Clears the interrupt an internal pin
/// @param pin The pin (<9 may damage the inkplate)
/// @return True on success, otherwise false
bool pcal_ex_clear_int_pin_int(uint8_t pin);
/// @brief Gets the interrupt for an internal pin
/// @param pin The pin (<9 may damage the inkplate)
/// @return The interrupt pin or -1 on error
int pcal_ex_get_int_int(void);
/// @brief Sets the port for external pins
/// @param values the bitmask of values (pins < 9 may damage the device)
/// @return True on success, otherwise false
bool pcal_ex_set_port_int(uint16_t values);
/// @brief Gets the port for external pins
/// @return The bitmask of values, or -1 on error
int pcal_ex_get_port_int(void);

#ifdef __cplusplus
}
#endif
#endif // PCAL_EX_H