# Graph-N

A fast, lightweight native C++ client for [ComfyUI](https://comfy.org/).

**ComfyUI** ([GitHub](https://github.com/comfyanonymous/ComfyUI)) is a powerful node-based backend and GUI for Stable Diffusion and generative AI models.  

**Graph-N** is designed to provide a highly optimized, native user interface for these workflows, bypassing the overhead of heavy browser-based clients while maintaining deep integration via WebView2.

[![Graph-N Demo Video](https://img.youtube.com/vi/MmhJXxgUIqM/maxresdefault.jpg)](https://youtu.be/MmhJXxgUIqM)
▶️ **[Watch the Graph-N Demo on YouTube](https://youtu.be/MmhJXxgUIqM)**

---

## Prerequisites & Dependencies

To build Graph-N, you will need a **C++20 compatible compiler**.
To avoid compiler detection issues, it is highly recommended to perform all steps within the **x64 Native Tools Command Prompt for VS 2026**.

### 1. Clone the Repository
```bash
git clone --depth 1 https://github.com/graphn-app/Graph-N.git
cd Graph-N
```

### 2. Build Boost
We use a specific version of Boost and compile only the necessary headers to save time and space.

```bash
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
```

### 3. Fetch Dependencies
Download required third-party libraries.

```bash
git clone --depth 1 https://github.com/yhirose/cpp-httplib.git
git clone --depth 1 https://github.com/fmtlib/fmt.git
git clone --depth 1 https://github.com/google/googletest.git
git clone --depth 1 https://github.com/nlohmann/json.git
git clone --depth 1 https://github.com/SRombauts/SQLiteCpp.git
git clone --depth 1 https://github.com/nothings/stb.git	
git clone --depth 1 https://github.com/mackron/miniaudio.git
```

### 4. Generate Project and Build
Use CMake to generate the Visual Studio solution and build the project. 

```bash
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

---

## License

This open-source version of Graph-N is distributed under the GNU General Public License v3.0 (GPLv3). See the [EULA.txt](EULA.txt) file for more details.

Third-party libraries and assets used in this project are governed by their respective licenses (MIT, Boost, ISC, OFL, Public Domain). See the [THIRD-PARTY-NOTICES.txt](THIRD-PARTY-NOTICES.txt) file for the full list of copyrights.

---

## Links

* **YouTube:** [youtube.com/@graphn-app](https://www.youtube.com/@graphn-app)
* **Email:** [graph.n.app@gmail.com](mailto:graph.n.app@gmail.com)

### For donates:
* **GitHub Sponsors:** [github.com/sponsors/elgeminy](https://github.com/sponsors/elgeminy)
* **Ko-fi:** [ko-fi.com/graphn_app](https://ko-fi.com/graphn_app)
* **BTC:** `bc1q59rxm745wkwp6sxj36dhknh4253uvslrnfpy6c`
* **SOL:** `AiBADrwaHGTqt62yRw1NH8rkpZvrf631ZqazjB9ThM3k`