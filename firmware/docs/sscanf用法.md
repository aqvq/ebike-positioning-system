https://blog.csdn.net/hs_guanqi/article/details/2258558

https://www.cnblogs.com/zswbky/p/5432128.html#:~:text=%E5%9C%A8%E7%BB%A7%E7%BB%AD%E8%AE%A8%E8%AE%BA%E4%B9%8B%E5%89%8D%EF%BC%8C,%E5%AE%83%E6%8B%B7%E8%B4%9D%E7%BB%99str%E3%80%82

```c
sscanf (buf, "%02X:02X:02X:02X:02X:02X",
        &mac[0],
        &mac[1],
        &mac[2],
        &mac[3],
        &mac[4],
        &mac[5]
    );
```

```c
"%hhX:hhX:hhX:hhX:hhX:hhX"
```