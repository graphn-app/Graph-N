Graph-N

A fast, lightweight native C++ client for ComfyUI.

Graph-N is designed to provide a highly optimized, native user interface for ComfyUI workflows, bypassing the overhead of heavy browser-based clients while maintaining deep integration via WebView2.

===============================================================================
Prerequisites & Dependencies

To build Graph-N, you will need a C++20 compatible compiler
To avoid compiler detection issues, it is highly recommended to perform all steps within the x64 Native Tools Command Prompt for VS 2026.

1. Clone the Repository
git clone --depth 1 https://github.com/graphn-app/Graph-N.git
cd Graph-N

2. Build Boost
We use a specific version of Boost and compile only the necessary headers to save time and space.

git clone --depth 1 --branch boost-1.90.0 https://github.com/boostorg/boost.git
cd boost
git submodule update --init tools/boostdep
git submodule update --init libs/beast
python tools/boostdep/depinst/depinst.py beast	
python tools/boostdep/depinst/depinst.py asio
git submodule update --init libs/headers
.\bootstrap.bat msvc
.\b2 headers
cd ..

3. Fetch Dependencies
Download required third-party libraries.

git clone --depth 1 https://github.com/yhirose/cpp-httplib.git
git clone --depth 1 https://github.com/fmtlib/fmt.git
git clone --depth 1 https://github.com/google/googletest.git
git clone --depth 1 https://github.com/nlohmann/json.git
git clone --depth 1 https://github.com/SRombauts/SQLiteCpp.git
git clone --depth 1 https://github.com/nothings/stb.git	
git clone --depth 1 https://github.com/mackron/miniaudio.git

4. Generate Project and Build
Use CMake to generate the Visual Studio solution and build the project 

cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release

===============================================================================
Privacy & Telemetry

Graph-N collects basic, completely anonymous telemetry data (such as hardware specifications, screen resolution, and session duration) to help improve the application. We do not collect, read, or transmit your personal data, prompts, generated images, or API keys.

For full details, please read our PRIVACY.md.

===============================================================================
License

This open-source version of Graph-N is distributed under the GNU General Public License v3.0 (GPLv3). See the LICENSE.txt file for more details.

Third-party libraries and assets used in this project are governed by their respective licenses (MIT, Boost, ISC, OFL, Public Domain). See the THIRD-PARTY-NOTICES.txt file for the full list of copyrights.

