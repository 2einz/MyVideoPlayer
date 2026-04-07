
---

# 开发环境与 FFmpeg 技术笔记

## 一、 Qt 环境配置与部署

### 1.1 自动化部署工具
在 Windows 平台下，使用 Qt 自带的 `windeployqt` 可以自动补全程序运行所需的动态库（DLL）。

**使用命令：**
```bash
# 建议在 Qt 命令行终端中运行，以确保环境变量正确
# 路径格式：[windeployqt路径] [可执行文件路径]
D:\Qt\6.11.0\msvc2022_64\bin\windeployqt.exe Project.exe
```

### 1.2 编译器选择（重要）
> [!CAUTION]
> **在 MSVC 环境下开发时，请勿使用 Clang 编译器。**
> 混合使用可能导致 ABI 不兼容，引发不可预知的链接错误或运行时崩溃。请统一使用原生 MSVC 编译器。

---

## 二、 FFmpeg 开发笔记

本章节记录 FFmpeg API 调用过程中的核心细节与坑点。

### 2.1 错误处理机制
在 FFmpeg 中，返回值通常为负数代表错误。解析错误信息主要有两种方式：

#### ① `av_err2str` (宏)
*   **定义**：一个封装好的宏，内部使用了临时数组。
*   **适用场景**：仅用于 `printf` 或 `log` 内部临时打印。
*   **代码示例**：
    ```c
    // 方便快捷，但生命周期仅限于这一行
    printf("error: %s\n", av_err2str(ret));
    ```
*   **注意**：不要试图用指针保存其返回值，否则会指向已销毁的临时内存。

#### ② `av_strerror` (函数)
*   **定义**：标准的 API 函数。
*   **适用场景**：需要将错误信息拷贝到持久化缓冲区时使用。
*   **代码示例**：
    ```c
    char err_buf[128] = {0};
    av_strerror(ret, err_buf, sizeof(err_buf));
    printf("error: %s\n", err_buf);
    ```

---

### 2.2 内存管理（待补充）
*   *提示：后续可在此处添加关于 av_malloc, av_frame_alloc 等内容...*

### 2.3 常用结构体说明（待补充）
*   *提示：后续可在此处添加关于 AVFormatContext, AVCodecContext 等内容...*

### 2.4 常见问题处理（FAQ）
*   **Q: 为什么某些格式无法解码？**
    *   A: 检查是否执行了网络初始化 `avformat_network_init()` 或检查配置编译脚本。

---
---