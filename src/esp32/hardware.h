#ifndef HARDWARE_H
#define HARDWARE_H

#if defined(INKPLATE10) || defined(INKPLATE10V2)
#define I2C_PORT I2C_NUM_0
#define I2C_SDA 21
#define I2C_SCL 22

#define SPI_PORT SPI3_HOST
#define SPI_MISO 12
#define SPI_MOSI 13
#define SPI_CLK 14

#define SD_CS 15
#define SD_PORT SPI_PORT
#endif // INKPLATE10

#ifdef M5STACK_CORE2
#define I2C_PORT I2C_NUM_1
#define I2C_SDA 21
#define I2C_SCL 22

#define SPI_PORT SPI3_HOST
#define SPI_CLK 18
#define SPI_MISO 38
#define SPI_MOSI 23

#define SD_PORT SPI3_HOST
#define SD_CS 4

#endif
#endif // HARDWARE_H