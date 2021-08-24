#include <common/Print.h>
#include <common/Assert.h>

#include <scene/GLTFModel.h>


void GLTFModel::load(const std::string& path) {
    tinygltf::TinyGLTF loader;

    std::string err, warn;

    // parse the gltf file
    if (!loader.LoadASCIIFromFile(&model, &err, &warn, path)) {
        throw std::runtime_error("Could not parse .gltf file");
    }

    if (!warn.empty()) {
        print(warn.c_str());
    }

    if (!err.empty()) {
        throw std::runtime_error(err);
    }

    // generate materials
}