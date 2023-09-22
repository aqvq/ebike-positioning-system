## README

此文件夹实现log函数功能，实现LOG日志打印，支持彩色打印，支持分级：

- LOGE: ERROR
- LOGW: WARNING
- LOGI: INFO
- LOGD: DEBUG
- LOGT: TRACE

优先级依次降低。ERROR最高，TRACE最低。

如果未定义DEBUG宏，则不会输出LOGD和LOGT中的内容，减少日志输出内容。

也可在`log.h`中单独控制某个分级的输出开关。
