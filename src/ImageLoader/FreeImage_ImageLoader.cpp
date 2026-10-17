#include "./FreeImage_ImageLoader.h"
#include "../DebugTools/OutStream.h"
#include <iostream>
#include <stdexcept>

namespace FCT {
    void FreeImage_ImageLoader::Init()
    {
        FreeImage_Initialise();
        FreeImage_SetOutputMessage(freeImageErrorHandler);
    }
    void FreeImage_ImageLoader::Tern()
    {
        FreeImage_DeInitialise();
    }
    FreeImage_ImageLoader::FreeImage_ImageLoader() {
    }

    FreeImage_ImageLoader::~FreeImage_ImageLoader() {
    }

    ImageLoader::ImageData FreeImage_ImageLoader::loadFromMemory(const unsigned char* data, size_t size) {
        ZoneScoped();
        FIMEMORY* hmem = FreeImage_OpenMemory((BYTE*)data, size);
        if (hmem == nullptr) {
            throw std::runtime_error("Could not open memory stream for image");
        }

        FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(hmem, 0);
        if (fif == FIF_UNKNOWN) {
            FreeImage_CloseMemory(hmem);
            throw std::runtime_error("Unknown image format from memory");
        }

        FIBITMAP* dib = FreeImage_LoadFromMemory(fif, hmem, 0);
        FreeImage_CloseMemory(hmem); // Memory handle can be closed after loading

        if (!dib) {
            throw std::runtime_error("Failed to load image from memory");
        }

        FIBITMAP* dibConverted = FreeImage_ConvertTo32Bits(dib);
        FreeImage_Unload(dib);

        if (!dibConverted) {
            throw std::runtime_error("Failed to convert image to 32 bits");
        }

        ImageData imageData;
        imageData.width = FreeImage_GetWidth(dibConverted);
        imageData.height = FreeImage_GetHeight(dibConverted);
        imageData.channels = 4; // RGBA

        unsigned int pitch = FreeImage_GetPitch(dibConverted);
        imageData.data.resize(imageData.height * pitch);

        FreeImage_ConvertToRawBits(imageData.data.data(), dibConverted, pitch, 32,
            FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);

        // FreeImage loads images as BGRA, so we need to swap Red and Blue channels to get RGBA.
        for (unsigned int y = 0; y < imageData.height; ++y) {
            for (unsigned int x = 0; x < imageData.width; ++x) {
                unsigned int offset = (y * pitch) + (x * 4);
                std::swap(imageData.data[offset], imageData.data[offset + 2]);
            }
        }

        FreeImage_Unload(dibConverted);

        return imageData;
    }


    ImageLoader::ImageData FreeImage_ImageLoader::load(const std::string& filename) {
        ZoneScoped();
        ZoneText(filename.c_str(), filename.size());
        FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(filename.c_str(), 0);
        if (fif == FIF_UNKNOWN) {
            fif = FreeImage_GetFIFFromFilename(filename.c_str());
        }

        if (fif == FIF_UNKNOWN) {
            throw std::runtime_error("Unknown image format");
        }

        FIBITMAP* dib = FreeImage_Load(fif, filename.c_str());
        if (!dib) {
            ferr << "Failed to load image: " + filename << std::endl;
            return {};
            //throw std::runtime_error("Failed to load image: " + filename);
        }

        FIBITMAP* dibConverted = FreeImage_ConvertTo32Bits(dib);
        FreeImage_Unload(dib);

        if (!dibConverted) {
            throw std::runtime_error("Failed to convert image to 32 bits");
        }

        ImageData imageData;
        imageData.width = FreeImage_GetWidth(dibConverted);
        imageData.height = FreeImage_GetHeight(dibConverted);
        imageData.channels = 4; // RGBA

        unsigned int pitch = FreeImage_GetPitch(dibConverted);
        imageData.data.resize(imageData.height * pitch);

        FreeImage_ConvertToRawBits(imageData.data.data(), dibConverted, pitch, 32,
            FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);
        
        for (unsigned int y = 0; y < imageData.height; ++y) {
            for (unsigned int x = 0; x < imageData.width; ++x) {
                unsigned int offset = (y * pitch) + (x * 4); 
                std::swap(imageData.data[offset], imageData.data[offset + 2]);
            }
        }


        FreeImage_Unload(dibConverted);

        return imageData;
    }

    bool FreeImage_ImageLoader::save(const std::string& filename, const ImageData& imageData) {
        ZoneScoped();
        FIBITMAP* dib = FreeImage_AllocateT(FIT_BITMAP, imageData.width, imageData.height, 32);
        if (!dib) {
            return false;
        }

        unsigned int pitch = FreeImage_GetPitch(dib);
        FreeImage_ConvertFromRawBits(
            (BYTE*)imageData.data.data(),
            imageData.width,
            imageData.height,
            pitch,
            32,
            FI_RGBA_RED_MASK,
            FI_RGBA_GREEN_MASK,
            FI_RGBA_BLUE_MASK
        );

        FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(filename.c_str());
        bool success = FreeImage_Save(fif, dib, filename.c_str());

        FreeImage_Unload(dib);

        return success;
    }

    std::vector<std::string> FreeImage_ImageLoader::getSupportedExtensions() const {
        return {".bmp", ".png", ".jpg", ".jpeg", ".tif", ".tiff"}; // Add more as needed
    }

    void FreeImage_ImageLoader::freeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char *message) {
        std::cerr << "FreeImage error: ";
        if (fif != FIF_UNKNOWN) {
            std::cerr << FreeImage_GetFormatFromFIF(fif) << " Format - ";
        }
        std::cerr << message << std::endl;
    }

} // namespace FCT