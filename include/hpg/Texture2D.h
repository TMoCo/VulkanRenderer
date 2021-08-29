///////////////////////////////////////////////////////
// Texture class declaration
///////////////////////////////////////////////////////

//
// A texture class for handling texture related operations and data.
//

#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#include <hpg/Texture.h>

class Texture2D : public Texture {
public:
    bool uploadToGpu(const Renderer& renderer, const ImageData& imageData);
};

#endif // !TEXTURE2D_H
