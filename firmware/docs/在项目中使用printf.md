## 如何在项目中使用printf函数来向串口打印日志呢？

在`usart.c`文件最后加入如下代码：

```c

/* USER CODE BEGIN 1 */

#ifdef __GNUC__
int _write(int fd, char *ptr, int len)  
{  
  HAL_UART_Transmit(&huart3, (uint8_t*)ptr, len, 0xFFFF);
  return len;
}

#else

int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 0xFFFF); // 输出指向串口USART3
    return ch;
}

#endif

/* USER CODE END 1 */

```

注意，上述代码中的`huart3`记得换成自己相应的串口。然后在`usart.c`文件开头包含头文件

```c

/* USER CODE BEGIN 0 */

#include <stdio.h>

/* USER CODE END 0 */

```
