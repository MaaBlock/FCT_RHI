#include "./Stb_ImageLoader.h"
#include "../DebugTools/OutStream.h"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "../../../FCT_Base/ThirdParty/tracy/profiler/src/stb_image.h"

namespace FCT {

    ImageLoader::ImageData Stb_ImageLoader::loadFromMemory(const unsigned char* data, size_t size) {
        ImageData imageData{};
        int w = 0;
        int h = 0;
        int comp = 0;
        unsigned char* pixels = stbi_load_from_memory(data, static_cast<int>(size), &w, &h, &comp, 4);
        if (!pixels) {
            ferr << "Failed to load image from memory via stb_image" << std::endl;
            return imageData;
        }
        imageData.width = w;
        imageData.height = h;
        imageData.channels = 4;
        size_t byteCount = static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
        imageData.data.assign(pixels, pixels + byteCount);
        stbi_image_free(pixels);
        return imageData;
    }

    ImageLoader::ImageData Stb_ImageLoader::load(const std::string& filename) {
        ImageData imageData{};
        int w = 0;
        int h = 0;
        int comp = 0;
        unsigned char* pixels = stbi_load(filename.c_str(), &w, &h, &comp, 4);
        if (!pixels) {
            ferr << "Failed to load image: " << filename << std::endl;
            return imageData;
        }
        imageData.width = w;
        imageData.height = h;
        imageData.channels = 4;
        size_t byteCount = static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
        imageData.data.assign(pixels, pixels + byteCount);
        stbi_image_free(pixels);
        return imageData;
    }

    bool Stb_ImageLoader::save(const std::string& filename, const ImageData& imageData) {
        (void)filename;
        (void)imageData;
        return false;
    }

    std::vector<std::string> Stb_ImageLoader::getSupportedExtensions() const {
        return {".bmp", ".png", ".jpg", ".jpeg", ".tga", ".hdr"};
    }

} // namespace FCT
