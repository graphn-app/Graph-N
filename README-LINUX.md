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
sudo apt-get install -y build-essential cmake git \
  libgl1-mesa-dev libglfw3-dev libcurl4-openssl-dev \
  libboost-dev libboost-system-dev libboost-beast-dev libboost-asio-dev \
  libfmt-dev libsqlite3-dev libminiaudio-dev

# Fedora
sudo dnf install -y gcc gcc-c++ make cmake git \
  mesa-libGL-devel glfw-devel libcurl-devel \
  boost-devel fmt-devel sqlite-devel miniaudio-devel
```

### 3. Fetch Third-Party Dependencies
Clone the bundled third-party libraries into the repository root. These are gitignored so they are not part of the repository itself.

```bash
git clone --depth 1 https://github.com/boostorg/boost.git boost
git clone --depth 1 https://github.com/yhirose/cpp-httplib.git
git clone --depth 1 https://github.com/fmtlib/fmt.git
git clone --depth 1 https://github.com/google/googletest.git
git clone --depth 1 https://github.com/nlohmann/json.git
git clone --depth 1 https://github.com/SRombauts/SQLiteCpp.git
git clone --depth 1 https://github.com/nothings/stb.git stb
git clone --depth 1 https://github.com/mackron/miniaudio.git
```

> **Note:** `boost` is cloned into `boost/` (provides `beast` and `asio` headers). `sqlite/` must contain the SQLite amalgamation (`sqlite3.c` / `sqlite3.h`) — obtain it from [sqlite.org](https://www.sqlite.org/download.html) and place the files in `sqlite/`; it is built as the `sqlite3` static library.
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
