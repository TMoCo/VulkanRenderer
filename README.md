# Vulkan Renderer application

Building on past projects, my aim here is to create an application for rendering to be used in a small game engine at some point in the future.

## Current features:
- Deferred rendering
- shadow mapping (point light)
- skybox
- textured model loading
- physically based shading (cook-torrance brdf with a selection of distribution functions)

## Before adding new features:
- [x] sort out command buffers
- [x] sort out render pass, make use of subpasses and subpass dependencies
- [x] sort out attachments
- [x] fix rotations

## New features:
- [ ] improve shadows (shadow cascades, omni-directional and directional light sources)
- [ ] post processing of final image (High dynamic range lighting and bloom)
- [ ] basic material system (revise descriptor sets and pipelines)
- [ ] Screen space ambient occlusion

This is a long term project (like a lot of my projects). 
Conceptually, these features are not difficult to understand but adapting them to Vulkan adds some overhead to development time. 

## Libraries I am using:
Developing on windows visual studio 2019, C++ 17.
* ImGui
* GLM
* tinygltf and tinyobj
* stbimage

## Links to helpful resources:
[lear opengl](https://learnopengl.com/) and [opengl tutorials](http://www.opengl-tutorial.org/) Understanding conceprtually in OpenGL helps.
[Vulkan example](https://github.com/SaschaWillems/Vulkan), excellent examples of important graphics techniques in Vulkan
[Vulkan tutorial](https://vulkan-tutorial.com/Introduction).
