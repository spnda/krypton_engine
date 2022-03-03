# rapi

rapi is the **R**endering **API** for Krypton. This allows the engine to interact
with the rendering in an abstract way and allows it to dynamically switch rendering
backends. There are currently two backends, a Vulkan backend for Linux and Windows,
and a Metal backend for MacOS. It also provides the Window through glfw. It incorporates
many vast systems, including render objects, materials, and textures.

# Render object system

Render objects are some mesh/object in 2D/3D space. The RAPI allows for instanced rendering, so
that meshes can be reused. However, this is up to the user to utilize. Firstly, render objects have
to be created and built. These methods are thread-safe, allowing for multiple render objects to be
loaded and built simultaneously.

```cpp
// Obtain a simple render object handle
auto handle = rapi->createRenderObject();

for (auto& prim : mesh.primitives) {
	// Add each primitive (essentially parts of a mesh) to the render object.
	// Each primitive can have a different material.
	rapi->addPrimitive(handle, prim, materialHandle);
}

// We can optionally set a custom transform on the mesh.
rapi->setObjectTransform(handle, glm::mat4(1.0));

// This finalizes the setup process. When new primitives are added, materials changed,
// or any other changes done to the render object, this has to be called again.
rapi->buildRenderObject(handle);
```

Inside of the render loop, it is now possible to render this render object. The caller does not
have any context for the render object itself and it is entirely possible the building has not
completed yet. This is especially notable when mesh loading is done on a separate thread. The RAPI
backend will itself check if the objects are ready and will only render them if so. Big objects
might take a bit of time to be built, causing the rendering to be slightly delayed.

# Material system

Materials are dynamic and can be assigned to each mesh primitive individually. They hold data
on the colors and all relevant textures, e.g. diffuse, roughness, occlusion, emissive, ...
Shaders can sample all of this data and utilize them, if the textures are all given. Materials
do not have to be built and can be updated at any time, however this does not allow the switching
between values within the same frame, as values are only updated once at the start of the frame.

```cpp
auto handle = rapi->createMaterial();
rapi->setBaseColor(handle, glm::vec3(0.5, 0.0, 1.0));
```

# Handles

Handles obtained from the RAPI interface are ref counted and the resource will live as long as it
is in use. Say you call `rapi->destroyMaterial(handle)` but the material is still bound to a primitive,
the material will actually not be destroyed and will be kept alive so that the primitive can be rendered
normally. This is particularly crucial when considering that materials and other resources are kept track
of through "slotmaps" or "free lists" and that there might be a new material placed in the same slot,
breaking rendering of any primitive using the old material.

We implement handles in a thread-safe way. For this, we use the `ReferenceCounter` class, which holds an
atomic counter. The counter is incremented when the handle is created and decremented when the handle is
destroyed. When the counter reaches 0, the resource behind the handle is destroyed. Every created handle
holds a pointer to a `ReferenceCounter` object, so that handle copies are respected.

A possible ref-counted implementation can be found in src/util/include/util/handle.hpp. It uses a
`std::shared_ptr` to hold the reference counter over multiple threads. It also implements copy and move
operations to ensure that ref counts are properly updated. The handles have a template argumnet parameter
to allow for different types of handles, which are simply separated by a string argument.

```cpp
krypton::util::Handle<"Material"> materialHandle;
```
