# TODO

This is essentially just a list of ideas and concepts I want to implement into this project. These
things may or may not happen and might also have different priorities.

## The list

- Make an abstraction for pipelines in the RAPI. This will decouple render passes and pipelines
  which allows the user to more quickly switch between pipelines seamlessly.
- Implement a SPIR-V validator to validate the SPIR-V passed to the RAPI, and whether it targets
  the correct shader stage and has a given entry point.
- Add a semaphore abstraction which bases on MTLEvent and Vulkan timeline semaphores.
- ???
