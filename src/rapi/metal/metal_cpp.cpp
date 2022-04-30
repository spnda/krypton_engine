#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

// I've put this into a seperate TU so that they don't have to be recompiled everytime the Metal
// backend gets modified.
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <rapi/metal/CAMetalLayer.hpp>
