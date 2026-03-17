# Explore2 — Space Exploration Engine

A real-time 3D space exploration engine written in C++, featuring GPU-accelerated procedural planet generation, physically-based atmospheric scattering, and a deferred rendering pipeline. Originally developed circa 2012–2014.

---

## Technical Highlights

### Deferred Rendering Pipeline
- Full G-Buffer (albedo, normal, specular, position, depth) with separate geometry and lighting passes
- Multi-stage render loop: geometry → lighting → atmospheric scatter → post-processing
- Shadow mapping with dedicated depth FBO and light frustum culling
- XML-defined effect/shader system with material state and parameter bindings

### GPU-Accelerated Procedural Generation (OpenCL)
- Perlin noise generated entirely on the GPU via custom OpenCL kernels
- Millions of terrain height samples computed in parallel per frame to feed the LOD system
- Custom OpenCL wrapper library (`HPCLib`) with templated buffer management, async execution, event callbacks, and platform/device enumeration
- Both float and double-precision kernel support

### Chunked LOD Terrain
- Hierarchical quad-tree spatial decomposition with distance-based error thresholds
- Dynamic chunk creation/deletion driven by view-dependent geometric error
- Smooth LOD morphing via per-vertex blend weights; vertex stitching eliminates seams
- Geometry batch caching: stable chunks are packed into vertex/texture atlases to minimise draw calls
- Asynchronous geometry building via thread pool with free-space recycling

### Physically-Based Atmospheric Scattering
- Rayleigh–Mie scattering with precomputed optical depth and phase function lookup tables
- Atmosphere modelled at ~80 km scale height with configurable density ratios
- Deferred scatter pass applied over scene geometry; depth-aware attenuation for visibility fading

### Orbital Mechanics & Solar System
- Multi-body celestial system: planets, moons, suns with configurable Keplerian orbits (semi-major axis, eccentricity, inclination, axial tilt)
- Time-based ephemeris updates; double-precision arithmetic via `hpalib` to prevent numerical drift at astronomical scales
- Planet visibility culling, distant planet impostor rendering, ray-sphere collision queries

### Procedural Noise & Texture Splatting
- Ridged multifractal noise (Kenton Musgrave algorithm) with SSE-optimised CPU evaluation
- Procedural texture splatting: three-way blend driven by height and polar angle, computed with SSE intrinsics
- Dynamic normal map and colour map generation via FBO rendering

### Python Scripting Integration
- Boost.Python bindings expose the Vice scene/animation system to Python 3
- Script context management with scene-aware reload and logging infrastructure

### Math & Scene Architecture
- Custom SSE-optimised math library: vec2/3/4, mat3/4, quaternions, Hermite interpolation, frustum culling, AABB/sphere tests
- Hierarchical transform-graph scene (group, drawable, camera nodes)
- Multiple camera types with viewport management

---

## Technology Stack

| Layer | Technology |
|-------|-----------|
| Graphics API | OpenGL 4.x (GLEW) |
| GPU Compute | OpenCL 1.0+ (via CUDA driver) |
| Windowing | SDL2 |
| UI / tools | Qt 5.15 |
| Scripting | Boost.Python 1.90 / Python 3.12 |
| Image loading | DevIL |
| Data serialization | TinyXML |
| Build system | CMake 3.15+ / vcpkg |
| Language | C++17 |

---

## Building

```
cmake --preset windows-debug
cmake --build build
```

Requires: Qt 5.15.2 (`QTDIR`), CUDA 13.2 (OpenCL headers), vcpkg (`VCPKG_ROOT`). All other dependencies installed via `vcpkg.json` manifest.

**Run configuration:** set working directory to `Explore2/Explore2Game/` so the engine resolves `../Data/` asset paths correctly.
