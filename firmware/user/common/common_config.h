
#ifndef _COMMON_CONFIG_H_
#define _COMMON_CONFIG_H_

typedef uint8_t bool;
#define true                 1
#define false                0
#define NULL                 ((void *)0)

#define MAC_ADDR_LEN         (6)

#define RAM_HEAP_SIZE        (90 * 1024)
// #define CCMRAM_HEAP_SIZE     (64 * 1024)

#define RAM_START_ADDRESS    ((uint8_t *)0x20000000)
#define RAM_SIZE             (128 * 1024)
#define CCMRAM_START_ADDRESS ((uint8_t *)0x10000000)
#define CCMRAM_SIZE          (64 * 1024)

#define CCMRAM               __attribute__((section(".ccmram")))
#define RAM                  __attribute__((section(".ram")))

#endif // _COMMON_CONFIG_H_
