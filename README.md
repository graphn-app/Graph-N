Graph-N

A fast, lightweight native C++ client for ComfyUI.

Graph-N is designed to provide a highly optimized, native user interface for ComfyUI workflows, bypassing the overhead of heavy browser-based clients while maintaining deep integration via WebView2.

Features

Native Performance: Built from the ground up using modern C++ and Dear ImGui for zero-lag interactions and minimal memory footprint.
WebView2 Integration: Seamlessly embeds browser technologies where necessary without sacrificing desktop app performance.
Local Session Management: Fast and reliable internal state saving using SQLite.

Prerequisites & Dependencies

To build Graph-N, you will need a C++20 compatible compiler and the following core components:

Dear ImGui & ImGui Node Editor
Microsoft Edge WebView2 SDK
SQLiteCpp
nlohmann/json
cpp-httplib

TODO: Add specific CMake or Visual Studio build instructions here

Privacy & Telemetry

Graph-N collects basic, completely anonymous telemetry data (such as hardware specifications, screen resolution, and session duration) to help improve the application. We do not collect, read, or transmit your personal data, prompts, generated images, or API keys.

For full details, please read our PRIVACY.md.

License

This open-source version of Graph-N is distributed under the GNU General Public License v3.0 (GPLv3). See the LICENSE.txt file for more details.

Third-party libraries and assets used in this project are governed by their respective licenses (MIT, Boost, ISC, OFL, Public Domain). See the THIRD-PARTY-NOTICES.txt file for the full list of copyrights.

