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

## Materials

Materials are dynamic and can be assigned to each mesh primitive individually. They hold data
on the colors and all relevant textures, e.g. diffuse, roughness, occlusion, emissive, ...
Shaders can sample all of this data and utilize them, if the textures are all given. Materials
do not have to be built and can be updated at any time, however this does not allow the switching
between values within the same frame, as values are only updated once at the start of the frame.

```cpp
auto handle = rapi->createMaterial();

// This sets the material base color.
rapi->setMaterialBaseColor(handle, glm::vec3(0.5, 0.0, 1.0));

// This binds the diffuseTexture to this material. Even if one should call destroyTexture
// on this diffuseTexture, it would not actually be destroyed as this material still exists.
rapi->setMaterialDiffuseTexture(handle, diffuseTexture);
```

## Textures

Textures are a crucial part for modern rendering. They incorporate color, roughness, depth and much
more. Within the RAPI, textures are supposed to be a static object. Uploading and editing textures
is a tedious and expensive process, requiring a lot of copying of potentially large data. Therefore,
textures are usually created once and never change.

```cpp
auto handle = rapi->createTexture();

// By default, textures are interpreted in linear encoding. This
// is not correct in terms of color textures, where the colors are actually
// in srgb colorspace.
rapi->setTextureColorEncoding(handle, kr::ColorEncoding::SRGB);

// We can pass a std::vector<std::byte> into the setTextureData function. The texture size has to
// be passed in here too. The last parameter describes how to interpret the texture data. The
// texture size has to be passed in here too. The texture format will usually be RGBA8, but can
// also be a compressed format like BC, ETC, or ASTC.
rapi->setTextureData(handle, 256, 256, textureData, kr::TextureFormat::RGBA8);

// The texture is uploaded. Textures can be reuploaded but it is strongly
// discouraged to do so.
rapi->uploadTexture(handle);
```

### Texture format

The texture format will usually be RGBA8 for textures loaded from PNG or JPEG files, but can also
be a compressed format like BC, ETC, or ASTC. Passing this value here is only telling the backend
how to interpret the texture data. The rendering backend will itself decide which version of these
compression formats to use, as some devices may support only some of them or only some of the
sub-formats and compressing them at runtime is very expensive and not recommended. For example,
desktop GPUs will usually support BCn, but not ETC2 nor ASTC, whereas mobile will support ETC2 and
ASTC, but not BCn.

For Vulkan, details on which devices support which compression format can be found
[on Sascha Willem's database](https://vulkan.gpuinfo.org/listoptimaltilingformats.php). This database
also includes a rough idea of what formats are supported by Metal devices, as MoltenVK can give a good
idea about what is supported and what not. It also includes metrics as to how many devices on a given
platform support which compression format which should help deciding which formats to use based on a
target platform.

## Handles

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

A possible ref-counted implementation can be found in `src/util/include/util/handle.hpp`. It uses a
`std::shared_ptr` to hold the reference counter over multiple threads. It also implements copy and
move operations to ensure that ref counts are properly updated. The handles have a template argument
parameter to allow for different types of handles, which are simply separated by a string argument
and are still fully typesafe.

```cpp
krypton::util::Handle<"Material"> materialHandle;
```
