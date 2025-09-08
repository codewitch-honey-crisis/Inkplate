#ifndef SPI_H
#define SPI_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
/// @brief Initializes the SPI bus
/// @return true on success, false on error
bool spi_init(void);
#ifdef __cplusplus
}
#endif
#endif // SPI_H