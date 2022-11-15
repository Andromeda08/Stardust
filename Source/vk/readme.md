### `re` Namespace
The `re` namespace - aside from new ones - contains components of the engine
which have been rewritten so far with the goal of making them more consistent, easier to use 
while providing more functionality.

- Buffers
  - Types: `VertexBuffer`, `IndexBuffer`, `UniformBuffer`, `InstanceBuffer`
  - Staging buffers can be created using `Buffer::make_staging_buffer()`
  - All buffer types now inherit from a universal base `Buffer` class. 
  - Various buffers such as (`VertexBuffer` and `UniformBuffer`) now accept
  custom data types as template parameters. (Such as in the `[BufferType]Data.hpp` files)
  - `IndexBuffer` now supports `uint32_t` and `uint16_t` index types.
  - Now support copying data to images.
- Meshes
  - A `Mesh` is now a mesh 😅
  - With `InstancedGeometry` we can do instanced rendering of meshes.
- Textures
  - With textures comes a new `Image` implementation
    - Reducing unnecessary code when splitting `Image` from `ImageView`
    - Supporting image layout transitions
  - Also samplers in `Sampler`.
- `Scene` right now only contains instanced geometries.

```c++
// Massive TODO: Split implementation from declarations.
```