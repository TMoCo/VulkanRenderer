///////////////////////////////////////////////////////
// TextureCube class declaration
///////////////////////////////////////////////////////

//
// A texture class for handling texture related operations and data.
//

#ifndef TEXTURECUBE_H
#define TEXTURECUBE_H

#include <hpg/Texture.h>

class TextureCube : public Texture {
public:
    bool uploadToGpu(const Renderer& renderer, const ImageData& imageData);
};

#endif // !TEXTURECUBE_H
