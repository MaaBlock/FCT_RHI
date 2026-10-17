#pragma once

#include "ImageLoader.h"

namespace FCT {

    class Stb_ImageLoader : public ImageLoader {
    public:
        Stb_ImageLoader() = default;
        ~Stb_ImageLoader() override = default;

        ImageData loadFromMemory(const unsigned char* data, size_t size) override;
        ImageData load(const std::string& filename) override;
        bool save(const std::string& filename, const ImageData& imageData) override;
        std::vector<std::string> getSupportedExtensions() const override;
    };

} // namespace FCT
