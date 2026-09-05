# AGENTS.md — MSIME-Server

产品级约定、跨仓契约与共享数据规则以 [MSIME-Windows 的 AGENTS.md](https://github.com/metasequoiaime/MSIME-Windows/blob/main/AGENTS.md) 为准，那一份统领整个水杉输入法项目，本仓的角色、IPC 协议、窗口与 WebView2 边界、DPI 约定都写在那里。本文件只补本仓内部容易踩空的几处。

本仓是常驻后端进程：输入引擎与候选状态、配置、词典、Named Pipe 服务，以及候选窗、悬浮工具栏、托盘菜单、设置窗口的原生宿主与 WebView2 控制器。

## vcpkg_installed 的解析

`CMakeLists.txt` 四处引用 `vcpkg_installed`，全部走 `${CMAKE_BINARY_DIR}`（`:257`、`:322`、`:369`、`:412`）。其中 `:369` 的 `WEBVIEW2_VCPKG_ROOT` 供 WebView2 loader 的导入库和运行时 DLL 拷贝使用。

**不要改回 `${CMAKE_SOURCE_DIR}/build/...`。** 那样 binary dir 就只能叫字面的 `build`，本仓自带的 `vcpkg-release` preset（`binaryDir` 是 `build-release`）和 `scripts/lcompile-release.ps1` 都会在链接 `MetasequoiaImeSettings` 时失败：

```
LINK : fatal error LNK1181: cannot open input file '...\build\vcpkg_installed\x64-windows\lib\WebView2Loader.dll.lib'
```

CI 刻意构建到 `build-release` 而不是 `build`，为的就是让这条不会悄悄退回去——两者在目录名恰好是 `build` 时解析结果相同，所以只有构建到别处才测得出来。

## 测试

`tests/CMakeLists.txt` 构建 `MetasequoiaImeServerTests`，源文件 34 个。根 `CMakeLists.txt` 结尾的 `if(BUILD_TESTING) add_subdirectory(tests) endif()` 把它接进主构建，`add_test` 在那个子目录里注册。

本地跑：

```powershell
ctest --test-dir build-release -C Release --output-on-failure
```

CI 现在也跑这一步。在此之前它只有 Configure 和 Build，测试**编译**得到检查而**断言**不会，所以一条失败的用例可以长期躺在 main 上而 CI 全绿。

有 7 个用例依赖**已安装的词库数据**（`others.db` 的 kaomoji 表、`msime.db`、`helpcodes/*.txt`），在裸机和 CI 上没有这些文件。它们通过 `test::require_data_files({...})` 在缺文件时报 `[SKIP]` 而不是 `[FAIL]`——在没有数据的机器上让它们失败，说明不了代码的任何问题。装了输入法的开发机上它们照常执行。

新写依赖词库的用例时沿用这个模式，不要让它在 CI 上红。

`src/ipc/ipc.h` 里有 14 条 `static_assert` 守着协议 ABI，那些是编译期的，构建时就会检查。

## uiAccess 与 Topmost 时序

`MetasequoiaImeServer.manifest` 用 `requestedExecutionLevel level="asInvoker" uiAccess="true"`，浮层才能盖在高完整性宿主之上。`uiAccess` 真正生效还依赖正确签名和安装在 Windows 认可的位置，清单、签名、安装路径是同一套发布约定。

进程在 `src/main.cpp:90` 设 `DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2`。

**创建 HWND 时不能直接带 `WS_EX_TOPMOST`。** 在 `uiAccess=true` 进程里，Topmost 父窗口会让 WebView2 的跨进程 `SetParent` 返回 `E_INVALIDARG`，或者形成「窗口存在、尺寸正确但永远空白」。现有顺序是：非 Topmost 建窗 → DWM cloak 下预热 → 三个共享 environment 的 controller 依次创建并完成首屏导航 → lazy-topmost gate 打开 → 分阶段提升到 `HWND_TOPMOST`。提升后要用 `RenotifyControllerAfterPin` 重新同步 bounds 和 parent position。不要在布局函数里随手传 `HWND_TOPMOST`，也不要绕过 `EnsureSmallWindowsTopmost` / lazy pin。

隐藏尚未完成首帧的 WebView2 host 会阻断 raster 初始化，预热阶段用 cloak，之后才按配置显示或隐藏。

## Boost

`CMakeLists.txt:13` 的 `Boost_ROOT` 是有守卫的：设了 `BOOST_ROOT` 环境变量就用环境变量，否则回退到作者本机路径。**这是有意的，不要改成只认环境变量**，那会破坏作者的本地环境。CI 就是靠设这个环境变量工作的。

Boost 是静态链接但没有写进 `vcpkg.json`，所以只能用 classic 模式装，且 triplet 必须是 `x64-windows-static-md` 而不是 `x64-windows-static`——`Boost_USE_STATIC_LIBS ON` 要静态 Boost 库，而项目其余部分用动态 CRT，纯 static triplet 会让 Boost 也切到静态 CRT，链接时报 LNK2038。

## 生成文件

`scripts/prepare_env.py` 会覆盖根目录的 `.clangd`、`CMakeLists.txt` 和 `CMakePresets.json`。长期改动要同步到 `scripts/config_files/` 下的同名模板，否则下次生成就丢了。该脚本按固定行号替换模板内容，调整模板前要一起检查脚本索引。

## 提交

提交信息用 `type(scope): 摘要`。不要添加 `Co-Authored-By`、`Generated with` 或其他 AI 生成标记。
