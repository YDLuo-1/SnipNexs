# Qt 6.11.2 runtime notices

SnipNexs dynamically links an unmodified Qt 6.11.2 build and deploys Qt Core,
Gui, Widgets, Network, the Windows platform plugin, the modern Windows style
plugin, and GIF/ICO/JPEG image-format plugins. Qt is available under the GNU
Lesser General Public License version 3 for this distribution.

The deployed Qt libraries contain or use third-party components documented by
the Qt Project, including:

- PCRE2 10.47, zlib 1.3.2, double-conversion 3.4.0, TinyCBOR 7.0, Unicode
  character data 36, CLDR data 48.2, SipHash, BLAKE2, MD4, MD5, SHA-1, SHA-3
  and Apache Tika MIME data;
- libpng 1.6.58, libjpeg-turbo 3.2.0, FreeType 2.14.3, HarfBuzz 14.3.0,
  Adobe Glyph List, Khronos OpenGL headers, D3D12 Memory Allocator, Vulkan
  Memory Allocator 3.2.1, MD4C 0.5.3, ICC color profiles, WebGradients and
  WinTab API;
- Public Suffix List data and libpsl.

These components retain their respective BSD, MIT, Apache-2.0, MPL-2.0, CC0,
Unicode, FreeType, libpng, IJG and other permissive notices. The exact
machine-readable package, versions, copyright statements and license
expressions are in `qtbase-6.11.2.spdx`, shipped next to this file. Complete
license texts and corresponding source are preserved in the unmodified
`qtbase-everywhere-src-6.11.2.tar.xz` attached to the matching GitHub Release.

Qt source copy:
<https://github.com/YDLuo-1/SnipNexs/releases/download/v0.7.0/qtbase-everywhere-src-6.11.2.tar.xz>

Official Qt third-party license index:
<https://doc.qt.io/qt-6.11/licenses-used-in-qt.html>

Qt was not modified by SnipNexs. Users may replace the Qt DLLs and plugins in
the application directory with an ABI-compatible Qt build. See
`THIRD_PARTY_NOTICES.md` in the package root for replacement instructions.
