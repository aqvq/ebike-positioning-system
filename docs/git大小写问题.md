首先git设置忽略大小写为false：

```bash
git config core.ignorecase false
```

然后删除远程仓库的原来不想要的文件夹或文件：

```bash
git rm --cached .\Libraries\linksdk -r
```

参考链接: [git服务器大小写两个文件,git 解决远程仓库文件大小写问题](https://blog.csdn.net/weixin_34582773/article/details/119659387#:~:text=%E7%94%A8git%E6%89%A7%E8%A1%8C%E4%B8%8B%E5%88%97%E5%91%BD%E4%BB%A4%EF%BC%9A%20%24%20git%20config%20core.ignorecase,false%20%E8%A7%A3%E9%87%8A%EF%BC%9A%E8%AE%BE%E7%BD%AE%E6%9C%AC%E5%9C%B0git%E7%8E%AF%E5%A2%83%E8%AF%86%E5%88%AB%E5%A4%A7%E5%B0%8F%E5%86%99%20%E4%BF%AE%E6%94%B9%E6%96%87%E4%BB%B6%E5%A4%B9%E5%90%8D%E7%A7%B0%EF%BC%8C%E5%85%A8%E9%83%A8%E6%94%B9%E4%B8%BA%E5%B0%8F%E5%86%99%20%28F2%E9%87%8D%E5%91%BD%E5%90%8D%E4%BF%AE%E6%94%B9%E5%8D%B3%E5%8F%AF%29%EF%BC%8C%E7%84%B6%E5%90%8Epush%E5%88%B0%E8%BF%9C%E7%A8%8B%E4%BB%93%E5%BA%93%E3%80%82%20%E8%BF%99%E6%97%B6%E5%A6%82%E6%88%91%E5%89%8D%E9%9D%A2%E7%9A%84%E5%9B%BE%E7%89%87%E6%89%80%E7%A4%BA%EF%BC%8C%E4%BB%93%E5%BA%93%E4%B8%8A%E5%B0%B1%E4%BC%9A%E6%9C%89%E9%87%8D%E5%90%8D%E7%9A%84%E6%96%87%E4%BB%B6%20%28%E6%96%87%E4%BB%B6%E5%A4%B9%29%E4%BA%86%E3%80%82)