# Third-party notices

## Premake

This project bundles the official `premake5.exe` (version 5.0.0), the Premake
Build Script Generator.

- Copyright (c) 2003-2026 Jess Perkins and individual contributors.
- License: BSD 3-Clause.
- Full license text: [`Scripts/Premake/LICENSE.txt`](Scripts/Premake/LICENSE.txt)
- Upstream: <https://github.com/premake/premake-core>

## Windows runtime libraries

The application links only against components shipped with Windows:

- Direct2D (`d2d1.dll`)
- DirectWrite (`DWrite.dll`)
- Desktop Window Manager (`dwmapi.dll`)
- Win32 (`user32.dll`, `gdi32.dll`, `shell32.dll`, `ole32.dll`, `kernel32.dll`)

No third-party DLLs are shipped with `dist\WorkEncourager.exe`.

## Inno Setup

The Windows installer is compiled with Inno Setup 6 (build-time tool).

- Copyright (c) 1997-2026 Jordan Russell and contributors.
- Website: <https://jrsoftware.org/isinfo.php>
- Inno Setup is free to use, including for commercial installers. The compiler
  is not bundled in this source repository; the compiled installer includes the
  Inno Setup runtime as permitted by its license.

## Chinese Simplified language file for Inno Setup

`installer/languages/ChineseSimplified.isl` is used only when building the
installer.

- Maintainer: Zhenghan Yang (Kira), <https://github.com/kira-96/Inno-Setup-Chinese-Simplified-Translation>
- Based on Inno Setup 6.5+ community translation.
- File attribution is also retained in the language file header.
