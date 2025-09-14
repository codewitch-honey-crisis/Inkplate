#ifndef I2C_H
#define I2C_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
/// @brief Initializes the I2C driver
/// @return True on success, otherwise false
bool i2c_init(void);
#ifdef __cplusplus
}
#endif
#endif // I2C_H