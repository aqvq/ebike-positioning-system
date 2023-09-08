#ifndef __LOGGER_H
#define __LOGGER_H

#include <stdio.h>
#include "main.h"

// 控制调试输出
#define LOG_OUPUT_ERROR
#define LOG_OUPUT_WARN
#define LOG_OUPUT_INFO
#define LOG_OUPUT_DEBUG
#define LOG_OUPUT_TRACE
#define LOG_COLOR_ENABLE

#ifdef LOG_COLOR_ENABLE
#define NONE      "\e[0m"
#define BLACK     "\e[0;30m"
#define L_BLACK   "\e[1;30m"
#define RED       "\e[0;31m"
#define L_RED     "\e[1;31m"
#define GREEN     "\e[0;32m"
#define L_GREEN   "\e[1;32m"
#define YELLOW    "\e[0;33m"
#define L_YELLOW  "\e[1;33m"
#define BLUE      "\e[0;34m"
#define L_BLUE    "\e[1;34m"
#define PURPLE    "\e[0;35m"
#define L_PURPLE  "\e[1;35m"
#define CYAN      "\e[0;36m"
#define L_CYAN    "\e[1;36m"
#define GRAY      "\e[0;37m"
#define L_GRAY    "\e[1;37m"
#define WHITE     "\e[1;37m"

#define BOLD      "\e[1m"
#define UNDERLINE "\e[4m"
#define BLINK     "\e[5m"
#define REVERSE   "\e[7m"
#define HIDE      "\e[8m"
#define CLEAR     "\e[2J"
#define CLRLINE   "\r\e[K"

#else

#define NONE      ""
#define BLACK     ""
#define L_BLACK   ""
#define RED       ""
#define L_RED     ""
#define GREEN     ""
#define L_GREEN   ""
#define YELLOW    ""
#define L_YELLOW  ""
#define BLUE      ""
#define L_BLUE    ""
#define PURPLE    ""
#define L_PURPLE  ""
#define CYAN      ""
#define L_CYAN      ""
#define GRAY      ""
#define L_GRAY    ""
#define WHITE     ""
#define L_WHITE     ""

#define BOLD      ""
#define UNDERLINE ""
#define BLINK     ""
#define REVERSE   ""
#define HIDE      ""
#define CLEAR     ""
#define CLRLINE   ""

#endif

#ifdef DEBUG

#ifdef LOG_OUPUT_ERROR
#define LOGE(TAG, FORMAT, ...) printf(RED "%010d [" L_RED "ERROR" RED "] " UNDERLINE "%s:%d:%s" RED " - " FORMAT "\r\n" NONE, HAL_GetTick(), __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define LOGE(TAG, FORMAT, ...) ;
#endif

#ifdef LOG_OUPUT_WARN
#define LOGW(TAG, FORMAT, ...) printf(YELLOW "%010d [" L_YELLOW "WARN" YELLOW "] " UNDERLINE "%s:%d:%s" YELLOW " - " FORMAT "\r\n" NONE, HAL_GetTick(), __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define LOGW(TAG, FORMAT, ...) ;
#endif

#ifdef LOG_OUPUT_INFO
#define LOGI(TAG, FORMAT, ...) printf(CYAN "%010d [" L_CYAN "INFO" CYAN "] " UNDERLINE "%s:%d:%s" CYAN " - " FORMAT "\r\n" NONE, HAL_GetTick(), __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define LOGI(TAG, FORMAT, ...) ;
#endif

#else

#ifdef LOG_OUPUT_ERROR
#define LOGE(TAG, FORMAT, ...) printf(RED "%010d [" L_RED "ERROR" RED "] %s - " FORMAT "\r\n" NONE, HAL_GetTick(), TAG, ##__VA_ARGS__)
#else
#define LOGE(TAG, FORMAT, ...) ;
#endif

#ifdef LOG_OUPUT_WARN
#define LOGW(TAG, FORMAT, ...) printf(YELLOW "%010d [" L_YELLOW "WARN" YELLOW "] %s - " FORMAT "\r\n" NONE, HAL_GetTick(), TAG, ##__VA_ARGS__)
#else
#define LOGW(TAG, FORMAT, ...) ;
#endif

#ifdef LOG_OUPUT_INFO
#define LOGI(TAG, FORMAT, ...) printf(CYAN "%010d [" L_CYAN "INFO" CYAN "] %s - " FORMAT "\r\n" NONE, HAL_GetTick(), TAG, ##__VA_ARGS__)
#else
#define LOGI(TAG, FORMAT, ...) ;
#endif

#endif

#ifdef LOG_OUPUT_DEBUG
#define LOGD(TAG, FORMAT, ...) printf(GREEN "%010d [" L_GREEN "DEBUG" GREEN "] " UNDERLINE "%s:%d:%s" GREEN " - " FORMAT "\r\n" NONE, HAL_GetTick(), __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define LOGD(TAG, FORMAT, ...) ;
#endif

#ifdef LOG_OUPUT_TRACE
#define LOGT(TAG, FORMAT, ...) printf(BLUE "%010d [" L_BLUE "TRACE" BLUE "] " UNDERLINE "%s:%d:%s" BLUE " - " FORMAT "\r\n" NONE, HAL_GetTick(), __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define LOGT(TAG, FORMAT, ...) ;
#endif

#define LOG(FORMAT, ...) printf(NONE FORMAT "\r\n", ##__VA_ARGS__)

#endif // __LOGGER_H
