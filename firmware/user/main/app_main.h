#ifndef __APP_MAIN_H
#define __APP_MAIN_H

void ec200_poweroff_and_mcu_restart(void);
void stm32_restart();
void cjson_init();
void print_welcome_message(void);
void get_running_version(void);

#endif // !__APP_MAIN_H
