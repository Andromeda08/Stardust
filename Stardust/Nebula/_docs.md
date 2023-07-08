## Nebula Docs

### `class Image`
- `struct ImageProperties`: Contains information of the image which don't change over its lifetime such as `vk::Format`.
- `struct ImageState`: Contains information that is subject to change over its lifetime such as `vk::ImageLayout`.
- The structs above and`vk::Image` and `vk::ImageView` objects can be accessed as `const&` using getter methods. 

### `namespace Nebula::Sync`
Requires the `synchronization2` extension which has been core since `Vulkan 1.3`.
- `class Barrier`: Defines the following interface for various Vulkan barriers.
  ```c++
  // Insert the specified barrier. (old -> new)
  virtual void apply(const vk::CommandBuffer& command_buffer);
  
  // Apply the inverse of the specified barrier. (new -> old)
  virtual void revert(const vk::CommandBuffer& command_buffer);
  
  // Executes the given lambda after inserting a barrier then immediately applies the inverse of the barrier.
  void wrap(const vk::CommandBuffer& command_buffer, const std::function<void()>& lambda);
  ```
- Operations related to images
  - `class ImageBarrier`: Wrapper for ImageMemoryBarrier objects, subclass of `Barrier`.
  - `class ImageBlit`: Wrapper for copying images.
  - `class ImageResolve`: Wrapper for resolving multisampled images.