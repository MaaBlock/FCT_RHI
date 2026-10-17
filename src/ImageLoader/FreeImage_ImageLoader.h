#pragma once

#include "ImageLoader.h"
#ifdef FCT_USE_FREEIMAGE
#include <FreeImage.h>

namespace FCT {

    class FreeImage_ImageLoader : public ImageLoader {
    public:
        static void Init();
        static void Tern();
        FreeImage_ImageLoader();
        ~FreeImage_ImageLoader();
        ImageLoader::ImageData loadFromMemory(const unsigned char* data, size_t size) override;
        ImageData load(const std::string& filename) override;
        bool save(const std::string& filename, const ImageData& imageData) override;
        std::vector<std::string> getSupportedExtensions() const override;

    private:
        static void freeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char *message);
    };

} // namespace FCT
#endif