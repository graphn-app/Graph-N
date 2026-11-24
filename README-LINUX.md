# Graph-N: building on Linux (in development)

Back to the [main README](README.md). Windows instructions: [README-WIN32.md](README-WIN32.md).

**Not available on Linux yet (in development):**

* MaskEditor and PosEditor (the host application is based on WebView2)
* File selection dialogs
* Video display/playback on a node
* ComfyUI process management (starting/stopping the local ComfyUI server)
* Update check (it requires an HTTPS request)
* Drag-and-drop

---

## Build (Linux / GNU x86_64)

Graph-N targets C++20 and builds natively on Linux with GNU GCC/Clang. GLFW + OpenGL are used for the ImGui backend on Unix.

### 1. Clone the Repository
```bash
git clone --depth 1 https://github.com/graphn-app/Graph-N.git
cd Graph-N
```

### 2. Install System Dependencies
Install the development libraries required to build Graph-N on Linux.

```bash
# Debian / Ubuntu
sudo apt-get update
sudo apt-get install -y build-essential cmake git python3 \
  libgl1-mesa-dev libglfw3-dev

# Fedora
sudo dnf install -y gcc gcc-c++ make cmake git python3 \
  mesa-libGL-devel glfw-devel
```

### 3. Fetch Third-Party Dependencies
Clone the bundled third-party libraries into the repository root. These are gitignored so they are not part of the repository itself.

```bash
git clone --depth 1 --branch boost-1.90.0 https://github.com/boostorg/boost.git boost
cd boost
git submodule update --init tools/boostdep
./bootstrap.sh
python3 tools/boostdep/depinst/depinst.py beast
python3 tools/boostdep/depinst/depinst.py asio
cd ..
git clone --depth 1 https://github.com/yhirose/cpp-httplib.git
git clone --depth 1 https://github.com/fmtlib/fmt.git
git clone --depth 1 https://github.com/google/googletest.git
git clone --depth 1 https://github.com/nlohmann/json.git
git clone --depth 1 https://github.com/SRombauts/SQLiteCpp.git
git clone --depth 1 https://github.com/nothings/stb.git stb
git clone --depth 1 https://github.com/mackron/miniaudio.git
git clone --depth 1 https://github.com/nothings/stb.git
```

> **Note:** `boost` is cloned into `boost/` (provides `beast` and `asio` headers). `sqlite/` is already in the repository (`sqlite3.c` / `sqlite3.h`) and is built as the `sqlite3` static library — no separate download needed.
>
> `imgui/` and `imgui-node-editor/` are already vendored in the repository, so they do not need to be cloned.

### 4. Configure and Build with CMake
Generate the build files and compile the `Graph-N` target in Release mode.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8 --target Graph-N
```

The `Graph-N` executable is output to `bin/Release/` (and copied to the repo root).

To run unit tests:
```bash
cmake --build build -j8 --target UnitTests
ctest --test-dir build --output-on-failure
```
