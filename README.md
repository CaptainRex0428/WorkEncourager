# 上班鼓励器 · Work Encourager

把今天的上班时间，实时换算成已经到账的钱。

[![Build](https://github.com/CaptainRex0428/WorkEncourager/actions/workflows/build.yml/badge.svg)](https://github.com/CaptainRex0428/WorkEncourager/actions/workflows/build.yml)
![Version](https://img.shields.io/badge/version-1.4.0-1D4ED8)
![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-0078D4)
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)

一个本地运行的 Windows 桌面小工具：把今天的上班时间，实时换算成已经到账的钱。
它不是考勤系统，也不是工资结算工具，只是一个陪你熬过下午四点的小窗口。

![主界面](docs/preview.png)

## 下载 / 安装

当前版本：**v1.4.0**（[更新日志](CHANGELOG.md)）

### 安装版（推荐）

**[下载安装包 WorkEncourager-Setup-1.4.0.exe](dist/WorkEncourager-Setup-1.4.0.exe)**  
Windows 10 / 11 x64 · 约 2.2 MB

双击安装包，按向导完成安装。默认安装到当前用户的：

```
%LOCALAPPDATA%\Programs\WorkEncourager
```

- 不需要管理员权限。
- 自动创建开始菜单快捷方式和卸载入口。
- 桌面快捷方式可选。
- 可从「设置 → 应用 → 已安装的应用」或开始菜单里的“卸载上班鼓励器”卸载。
- 卸载默认保留 `%LOCALAPPDATA%\WorkEncourager\settings.ini` 中的个人设置。
- 安装包未做代码签名，Windows SmartScreen 可能显示未知发布者提示。

### 便携版

**[下载 WorkEncourager.exe](dist/WorkEncourager.exe)**  
Windows 10 / 11 x64 · 约 326 KB · 静态 CRT，无需安装 VC++ 运行库。

下载后直接双击运行。也可以使用 [GitHub Releases](https://github.com/CaptainRex0428/WorkEncourager/releases) 中的发布资产；安装包、便携版和校验文件都会随版本标签由 CI 生成。

默认节奏：

- 月薪按平均 **21 个工作日/月** 折算成日薪，再按本班时长折算时薪。
- 默认显示今天实时累计的“已赚”、下班倒计时、工时进度和本月累计。
- 主界面固定展示 5 个代码内置的小目标，达成后自动变绿。
- 点击右上角齿轮可以调整上班时间、下班时间和月薪。
- 系统托盘常驻；点 `×` 只收起后台，不结束计时。
- 所有数据仅保存在本机。

设置界面：

![设置界面](docs/preview-settings.png)

## 功能

- **月薪换算**：`日薪 = 月薪 ÷ 21`，`折算时薪 = 日薪 ÷ 本班小时数`；实时速度再按秒拆分。
- **实时金额**：根据折算时薪逐秒累计，保留 4 位小数。
- **下班倒计时**：按系统本地时间计算，自动处理上班前、上班中、暂停、下班后和周末。
- **暂停计时**：休息时冻结今日已赚和工时进度；按钮变为“继续计时”。下班倒计时仍按真实时间走。
- **工时进度**：显示已工作 / 本班总时长和百分比进度条。
- **本月累计**：已过去的工作日按满勤日薪估算，再加上今天已赚的部分；周末不自动计薪。
- **五个内置小目标**：咖啡、午饭、T 恤、跑鞋、耳机。它们固定写在代码里，不通过设置维护。
- **本机持久化**：保存班次和月薪，重新打开后继续使用。
- **系统托盘**：常驻 Windows 通知区域；悬停显示自绘信息卡，包含上班状态、今日已赚和下班倒计时。左键唤起窗口，右键可暂停/继续或退出。关闭主窗口只会收起后台，不会结束进程。
- **精致小窗**：Direct2D + DirectWrite 自绘界面，圆角卡片、柔和背景、无系统标题栏。

## 内置小目标

目标在 [`BuiltInSmallGoals()`](src/WorkStats.cpp#L99-L108) 中写死，按价格从低到高展示：

| 小目标 | 价格 |
|---|---:|
| 咖啡 | ¥19 |
| 午饭 | ¥35 |
| T 恤 | ¥129 |
| 跑鞋 | ¥399 |
| 耳机 | ¥799 |

- 已经赚到的目标显示为绿色并带勾。
- 下一个还没达成的目标会高亮，顶部显示“还差 ¥x · 目标名称”。
- 卡片右上角显示“已达成 X / 5”。
- 想增减目标，直接改 `src/WorkStats.cpp` 里的 `BuiltInSmallGoals()` 数组即可，不需要改设置界面。

## 默认节奏

| 设置 | 默认值 |
|---|---|
| 上班时间 | 10:30 |
| 下班时间 | 18:00 |
| 本班时长 | 7 小时 30 分 |
| 月薪 | ¥2,715.30 |
| 平均工作日 | 21 天 / 月 |
| 满勤日薪 | ¥129.30 |
| 折算时薪 | ¥17.24 |

默认月薪反推自参考图的时薪：`17.24 × 7.5 小时 × 21 天 = 2715.30`。首次启动时不会生成设置文件；在设置里点“保存设置”后，才会写入：

```
%LOCALAPPDATA%\WorkEncourager\settings.ini
```

删除该文件即可恢复默认设置。

## 操作

- 拖动标题栏可以移动窗口。
- 点击右上角齿轮或按 `Ctrl + ,` 打开设置。
- 设置里用 `Tab` / `↑` / `↓` 切换输入框，`Enter` 保存，`Esc` 取消，`Ctrl + A` 全选当前输入框。
- 时间支持 `HH:MM`，也可以直接输入 `0900` 这样的四位数字，输入框会自动插入冒号。
- 点击“暂停计时器”开始休息，再点“继续计时”恢复。
- 右上角 `—` 最小化；`×` 收起主窗口并留在后台，托盘继续计时和刷新提示。
- 真正退出：右键托盘图标，选择“退出”。

## 系统托盘

应用启动后会在 Windows 通知区域注册一个图标：

- **悬停**：在图标上方显示自绘信息卡，例如  
  `上班鼓励器`  
  `上班中 · 今日已赚 ¥31.27`  
  `距离下班 5 小时 41 分`
- 信息卡不依赖系统托盘 tooltip，在 Windows 11 的隐藏图标浮层里也能显示；鼠标移开后自动消失。
- **左键单击**：打开主界面；主界面最小化或已收起时都会恢复窗口。
- **右键单击**：弹出菜单，可打开主界面、暂停/继续计时、退出。
- **关闭主窗口**：只是隐藏窗口，进程、计时和托盘提示都继续运行；再次左键托盘图标即可恢复。重复双击 exe 会唤醒已运行的实例，不会开出第二个后台。
- **退出应用**：托盘右键菜单里的“退出”。这是主动结束后台进程的入口。

Windows 11 新加入的托盘图标默认可能被收进「隐藏的图标」浮层（也就是你截图里的那种栏）。在浮层里把图标拖到任务栏可见区域即可固定。

## 构建

### 环境要求

- Windows 10 / 11 x64
- Visual Studio 2022，安装“使用 C++ 的桌面开发”工作负载
- 不需要额外安装 Premake，仓库里已经带了 Premake 5.0.0

### 一键构建

```bat
Scripts\BuildWIN.bat
```

脚本会依次完成：

1. 用 Premake 生成 `WorkEncourager.sln` 和 `settings\WorkEncourager.vcxproj`
2. 调用 MSBuild 编译 `Dist | x64`
3. 把可执行文件复制到 `dist\WorkEncourager.exe`
4. 生成 `dist\SHA256SUMS.txt` 校验文件

### 自动构建

仓库配置了 GitHub Actions：`.github/workflows/build.yml` 会在每次 push / pull request 时编译 `Dist | x64`、生成安装包，并上传构建产物。推送 `v*` 标签时会自动创建对应的 GitHub Release，并附加便携版、安装包和 SHA-256 校验文件。

### 构建安装包

安装 [Inno Setup 6](https://jrsoftware.org/isdl.php)，确保 `ISCC.exe` 在以下任一位置：

- `%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe`
- `%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe`
- `%ProgramFiles%\Inno Setup 6\ISCC.exe`
- `PATH`

然后运行：

```bat
Scripts\BuildInstaller.bat
```

脚本读取 `dist\WorkEncourager.exe`，生成：

```
dist\WorkEncourager-Setup-1.4.0.exe
```

并更新 `dist\SHA256SUMS.txt`。安装包的 Inno Setup 脚本位于 `installer\WorkEncourager.iss`，默认按用户安装，不请求管理员权限。

### 标准 Premake 调用

与 NCpp 的流程一致，先进入仓库根目录：

```bat
Scripts\Premake\premake5.exe --file=Build.lua vs2022
```

然后在 VS2022 中打开解决方案，选择 `Dist` + `x64` 编译；或者从 Developer Command Prompt：

```bat
msbuild WorkEncourager.sln /m /p:Configuration=Dist /p:Platform=x64
```

`Dist` 配置使用静态 CRT、开启优化、不生成调试符号；产物不依赖 MSVC 运行库 DLL。

### 输出位置

| 产物 | 路径 |
|---|---|
| 便携版可执行文件 | `dist\WorkEncourager.exe` |
| 安装包 | `dist\WorkEncourager-Setup-1.4.0.exe` |
| SHA-256 校验 | `dist\SHA256SUMS.txt` |
| Premake / MSBuild 原始输出 | `bin\windows-x86_64\Dist\WorkEncourager.exe` |
| 中间文件 | `bin\obj\windows-x86_64\Dist\WorkEncourager` |
| 生成工程 | `settings\` |

## 项目结构

```
.
├─ .github/workflows/build.yml  GitHub Actions 构建与标签发布
├─ .editorconfig             编辑器编码与换行约定
├─ .gitattributes            Git 文本 / 二进制属性
├─ Build.lua                 Premake workspace 入口
├─ Directory.lua             输出路径变量（沿用 NCpp 约定）
├─ Dependencies.lua          外部依赖声明（本项目无第三方库）
├─ EntryProject.lua          WindowedApp 工程定义
├─ CHANGELOG.md              版本更新记录
├─ LICENSE                   MIT License
├─ include/
│  ├─ AppSettings.h          设置、月薪换算、INI 读写接口
│  ├─ AppWindow.h            Win32 + Direct2D 窗口接口
│  ├─ Resource.h             资源 ID
│  └─ WorkStats.h            工时 / 金额 / 内置目标数据模型
├─ src/
│  ├─ main.cpp               wWinMain、DPI 与 COM 初始化、单实例
│  ├─ AppWindow.cpp          窗口过程、自绘界面、托盘与输入交互
│  ├─ AppSettings.cpp        设置保存、解析与旧配置迁移
│  └─ WorkStats.cpp          工作阶段、金额、本月累计、内置目标
├─ res/
│  ├─ app.rc                 图标和版本资源
│  ├─ app.ico                多尺寸应用图标
│  └─ app.manifest           Per-Monitor V2 DPI、asInvoker
├─ Scripts/
│  ├─ GenerateWIN.bat        仅生成 VS2022 解决方案
│  ├─ BuildWIN.bat           生成 + 编译 + 复制到 dist
│  ├─ BuildInstaller.bat     生成 Inno Setup 安装包
│  ├─ WriteChecksums.ps1     生成发布文件 SHA-256 校验
│  └─ Premake/premake5.exe   项目内置的 Premake 5.0.0
├─ installer/
│  ├─ WorkEncourager.iss     Inno Setup 安装脚本
│  └─ languages/             安装向导中文翻译
├─ docs/                     README 截图资源
└─ dist/
   ├─ WorkEncourager.exe     可直接运行的便携版
   ├─ WorkEncourager-Setup-1.4.0.exe  安装包
   └─ SHA256SUMS.txt        发布文件校验值
```

## 计算规则

- 只支持同一天内的班次，例如 `10:30 → 18:00`；不支持跨午夜班次。
- 日薪固定为 `月薪 ÷ 21`。折算时薪为 `日薪 ÷ 本班小时数`；本班时长变化也会改变折算时薪。
- “今日已赚”按本班的工作秒数累计；暂停会从今日已赚和工时进度里扣除暂停时长。
- “本月累计”只统计 1 号到昨天的工作日满勤金额，再加上今天已赚；不计算未来的工作日，不扣请假、缺勤或亏损。
- 暂未处理法定节假日和调休。周末自动显示休息状态，不累计当天收入。
- 金额是鼓励用的实时估算，不等于真实工资单。

## 旧版本配置

1.0 版本曾经保存 `hourlyRate`。新版读取旧配置时会自动折算为等价的月薪：

```
月薪 = 旧时薪 × 本班小时数 × 21
```

保存一次后，配置里只保留 `startMinutes`、`endMinutes`、`monthlySalary`。旧键会被清理。

## 技术栈

- C++20 / Win32
- Direct2D 硬件加速自绘
- DirectWrite 中文与等宽数字排版
- DWM 圆角、阴影与 Per-Monitor V2 DPI
- Premake 5.0.0 生成 VS2022 工程
- 静态 CRT 的独立 x64 可执行文件

## 第三方组件

Premake 以二进制形式内置，许可见 [`Scripts/Premake/LICENSE.txt`](Scripts/Premake/LICENSE.txt)。
更多说明见 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。

## 版本

- 当前版本：**v1.4.0**
- 版本标签：`v1.4.0`
- 更新日志：[`CHANGELOG.md`](CHANGELOG.md)
- 仓库：[github.com/CaptainRex0428/WorkEncourager](https://github.com/CaptainRex0428/WorkEncourager.git)

## License

[MIT](LICENSE) © 2026 CaptainRex0428
