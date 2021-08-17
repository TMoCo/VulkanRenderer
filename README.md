# Vulkan Renderer application

Building on past projects, my aim here is to create an application for rendering to be used in a small game engine. 

Before adding new features, my todo list consists of:
- [ ] sort out command buffers
- [ ] sort out render pass, make use of subpasses and subpass dependencies
- [ ] sort out attachments
* revise code to be more alignment aware
- [ ] fix rotations

Features I want to add:
- [ ] improve shadows (shadow cascades, omni-directional and directional light sources)
- [ ] post processing of final image (High dynamic range lighting and bloom)
- [ ] basic material system (revise descriptor sets and pipelines)
- [ ] Screen space ambient occlusion

I realise this will take some time, my aim is to have a good basis for building a game by the start of next month. 
Conceptually, these features are not difficult to understand, but adapting them to Vulkan does add quite a challenge. 
Nevertheless, I am sure that as I continue to add to this project my understanding of this framework will grow, along with my grasp of graphics and games engineering in general.

Libraries I am using:
* ImGui
* GLM
* tinygltf and tinyobj
* stbimage

Developing on windows visual studio 2019, C++ 17.

Links to helpful resources:
[lear opengl](https://learnopengl.com/), [opengl tutorial](http://www.opengl-tutorial.org/). Super handy for understanding graphics concepts.
[vulkan example](https://github.com/SaschaWillems/Vulkan). When vulkan just doesn't make sense to me, I look up these examples for guidance.
[vulkan tutorial](https://vulkan-tutorial.com/Introduction). How I started using vulkan.
