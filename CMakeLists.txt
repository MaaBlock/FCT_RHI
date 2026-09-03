include(platform/platform.cmake)
include(config/copy_headers.cmake)
include(config/set_utf8_compiler_encoding.cmake)


file(GLOB RUNTIME_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/Runtime/*.cpp"
)

file(GLOB RUNTIME_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/Runtime/*.h"
)

file(GLOB RUNTIME_ANDROID_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/Runtime/Android_*.cpp"
)

file(GLOB IMAGELOADER_FREETYPE_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/ImageLoader/FreeImage_*.cpp"
)

file(GLOB MODELLOADER_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/ModelLoader/*.cpp"
)

file(GLOB MODELLOADER_ASSIMP_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/ModelLoader/Assimp_*.cpp"
)
list(REMOVE_ITEM MODELLOADER_MODULE_SOURCES ${MODELLOADER_ASSIMP_MODULE_SOURCES})

list(REMOVE_ITEM RUNTIME_MODULE_SOURCES ${RUNTIME_ANDROID_MODULE_SOURCES})

file(GLOB RUNTIME_WIN32_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/Runtime/Win32_*.cpp"
)
list(REMOVE_ITEM RUNTIME_MODULE_SOURCES ${RUNTIME_WIN32_MODULE_SOURCES})


file(GLOB RUNTIME_LINUX_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/Runtime/Linux_*.cpp"
)
list(REMOVE_ITEM RUNTIME_MODULE_SOURCES ${RUNTIME_LINUX_MODULE_SOURCES})

file(GLOB RUNTIME_ANDROID_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/Runtime/Android_*.h"
)
list(REMOVE_ITEM RUNTIME_MODULE_HEADERS ${RUNTIME_ANDROID_MODULE_HEADERS})

file(GLOB CONTEXT_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/Context/*.cpp"
)
file(GLOB CONTEXT_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/Context/*.h"
        "./src/Context/*.hpp"
)

file(GLOB RHI_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/RHI/*.cpp"
)
file(GLOB RHI_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/RHI/*.h"
)
file(GLOB RHI_VULKAN_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/RHI/VK_*.cpp"
)
file(GLOB RHI_VULKAN_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/RHI/VK_*.h"
)
list(REMOVE_ITEM RHI_MODULE_SOURCES ${RHI_VULKAN_MODULE_SOURCES})
list(REMOVE_ITEM RHI_MODULE_HEADERS ${RHI_VULKAN_MODULE_HEADERS})
file(GLOB CONTEXT_VULKAN_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/Context/VK_*.cpp"
)
file(GLOB CONTEXT_VULKAN_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/Context/VK_*.h"
)
file(GLOB CONTEXT_SHARDEC_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/Context/Shaderc_*.cpp"
)

list(REMOVE_ITEM CONTEXT_MODULE_SOURCES ${CONTEXT_SHARDEC_MODULE_SOURCES})
file(GLOB DEBUGTOOLS_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/DebugTools/[a-zA-Z0-9]*.h"
)
file(GLOB DEBUGTOOLS_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/DebugTools/*.cpp"
)
file(GLOB DEBUGTOOLS_ANDROID_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/DebugTools/Android_*.cpp"
)
list(REMOVE_ITEM DEBUGTOOLS_MODULE_SOURCES ${DEBUGTOOLS_ANDROID_MODULE_SOURCES})

file(GLOB PATH_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/Path/*.cpp"
)
file(GLOB UI_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/UI/*.cpp"
)
file(GLOB UI_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/UI/*.h"
)

file(GLOB UI_GLFW_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/UI/GLFW_*.cpp"
)
file(GLOB UI_GLFW_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/UI/GLFW_*.h"
)

list(REMOVE_ITEM UI_MODULE_SOURCES ${UI_GLFW_MODULE_SOURCES})

file(GLOB UI_ANDROID_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/UI/Android_*.cpp"
)

list(REMOVE_ITEM UI_MODULE_SOURCES ${UI_ANDROID_MODULE_SOURCES})
file(GLOB UI_ANDROID_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/UI/Android_*.h"
)
list(REMOVE_ITEM UI_MODULE_HEADERS ${UI_ANDROID_MODULE_HEADERS})

file(GLOB VERTEXTEXT_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/VertexAndText/*.cpp"
)
file(GLOB MEMORY_MODULE_SOURCES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/VertexAndText/*.cpp"
)

file(GLOB MODELLOADER_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/ModelLoader/*.h"
)

file(GLOB IMAGELOADER_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/ImageLoader/*.h"
)

file(GLOB VERTEXTEXT_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/VertexAndText/*.h"
)

file(GLOB MEMORY_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/Memory/*.h"
)

file(GLOB BASE_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/Base/*.h"
)
file(GLOB TYPE_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/Type/*.h"
)

file(GLOB PATH_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/Path/*.h"
)

file(GLOB ADDITIONAL_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/*.h"
)

find_package(Boost REQUIRED COMPONENTS system thread locale serialization)
#find_package(harfbuzz CONFIG REQUIRED)
#find_package(Freetype REQUIRED)
#find_package(Bullet CONFIG REQUIRED)
if (FCT_ANDROID)
    set(CMAKE_CXX_STANDARD 20)
    find_package(VulkanHeaders CONFIG REQUIRED)
else()
    set(CMAKE_CXX_STANDARD 20)
endif ()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/work_contract work_contract)

add_library(FCT STATIC
    ${RUNTIME_MODULE_SOURCES}
    ${DEBUGTOOLS_MODULE_SOURCES}
    ${UI_MODULE_SOURCES}
    ${CONTEXT_MODULE_SOURCES}
    ${CONTEXT_VULKAN_MODULE_SOURCES}
    ${RHI_VULKAN_MODULE_SOURCES}
    ${RHI_MODULE_SOURCES}
    ${MODELLOADER_MODULE_SOURCES}
    ${MODELLOADER_ASSIMP_MODULE_SOURCES}
    ${VERTEXTEXT_MODULE_SOURCES}
    ${MEMORY_MODULE_SOURCES}
    src/headers.cpp
    ${CONTEXT_MODULE_HEADERS}
    ${RHI_VULKAN_MODULE_HEADERS}
    ${PATH_MODULE_HEADERS}
    ${PATH_MODULE_SOURCES}
)
target_sources(FCT PRIVATE
        ${CONTEXT_SHARDEC_MODULE_SOURCES}
)

if (FCT_WIN32)
    target_sources(FCT PRIVATE
        ${RUNTIME_WIN32_MODULE_SOURCES}
    )
endif()

if (FCT_LINUX)
    target_sources(FCT PRIVATE
        ${RUNTIME_LINUX_MODULE_SOURCES}
    )
endif()

if (FCT_ANDROID) 
    target_sources(FCT PRIVATE
        ${UI_ANDROID_MODULE_SOURCES}
        ${DEBUGTOOLS_ANDROID_MODULE_SOURCES}
    )
    target_link_libraries(FCT PUBLIC Vulkan::Headers)

else()
    target_sources(FCT PRIVATE
            ${UI_GLFW_MODULE_SOURCES}
            ${IMAGELOADER_FREETYPE_MODULE_SOURCES}
    )
    find_package(freeimage CONFIG REQUIRED)
    target_link_libraries(FCT PRIVATE freeimage::FreeImage freeimage::FreeImagePlus)

    find_package(glfw3 CONFIG REQUIRED)
    target_link_libraries(FCT PUBLIC glfw)
    
    find_package(unofficial-omniverse-physx-sdk CONFIG REQUIRED)
    target_link_libraries(FCT PUBLIC unofficial::omniverse-physx-sdk::sdk)
    
    # Optional: import the defined target to copy over the GPU acceleration libraries (3rd party provided by NVIDIA)
    if(TARGET unofficial::omniverse-physx-sdk::gpu-library)
        if(UNIX)
            # Add rpath setting to find .so libraries on unix based systems
            set_target_properties(FCT PROPERTIES
                BUILD_WITH_INSTALL_RPATH TRUE
                INSTALL_RPATH "$ORIGIN"
            )
        endif()
        add_custom_command(TARGET FCT POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_FILE:unofficial::omniverse-physx-sdk::gpu-library>
                        $<TARGET_FILE_DIR:FCT>)
        if(WIN32)
            add_custom_command(TARGET FCT POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            $<TARGET_FILE:unofficial::omniverse-physx-sdk::gpu-device-library>
                            $<TARGET_FILE_DIR:FCT>)
        endif()
    else()
        message(WARNING "GPU acceleration library target not defined - GPU acceleration will NOT be available!")
    endif()
endif()

find_package(Freetype REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)

find_package(unofficial-shaderc CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_hlsl CONFIG REQUIRED)
find_package(spirv_cross_cpp CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)
target_link_libraries(FCT PUBLIC
        work_contract
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-hlsl
        spirv-cross-cpp
        spirv-cross-reflect
        unofficial::shaderc::shaderc
        assimp::assimp
        Freetype::Freetype
        Boost::system
        Boost::thread
        Boost::locale
        Boost::serialization
        absl::any
        absl::log
        absl::base
        absl::bits
        absl::status
        absl::statusor
        absl::strings
)

file(GLOB THIRDPARTY_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/ThirdParty/*.h"
        "./src/ThirdParty/*.hpp"
)

file(GLOB MUTILTHREADBASE_MODULE_HEADERS
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        "./src/MutilThreadBase/*.h"
)

set(ALL_HEADER_FILES
        ${RUNTIME_MODULE_HEADERS}
        ${CONTEXT_MODULE_HEADERS}
        ${RHI_MODULE_HEADERS}
        ${RHI_VULKAN_MODULE_HEADERS}
        ${CONTEXT_VULKAN_MODULE_HEADERS}
        ${DEBUGTOOLS_MODULE_HEADERS}
        ${UI_MODULE_HEADERS}
        ${UI_GLFW_MODULE_HEADERS}
        ${UI_ANDROID_MODULE_HEADERS}
        ${RUNTIME_ANDROID_MODULE_HEADERS}
        ${ADDITIONAL_HEADERS}
        ${MODELLOADER_MODULE_HEADERS}
        ${IMAGELOADER_MODULE_HEADERS}
        ${VERTEXTEXT_MODULE_HEADERS}
        ${MEMORY_MODULE_HEADERS}
        ${BASE_MODULE_HEADERS}
        ${MUTILTHREADBASE_MODULE_HEADERS}
        ${THIRDPARTY_MODULE_HEADERS}
        ${TYPE_MODULE_HEADERS}
        ${PATH_MODULE_HEADERS}
)

add_copy_target_headers(FCT "${ALL_HEADER_FILES}")
set_utf8_for_target(FCT)
#
#file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/FCT)
#
#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/FCT.h ${CMAKE_CURRENT_SOURCE_DIR}/include/FCT.h COPYONLY)
#
#foreach(header_file ${ALL_HEADER_FILES})
#    file(RELATIVE_PATH relative_path ${CMAKE_CURRENT_SOURCE_DIR}/src ${header_file})
#    get_filename_component(header_dir ${relative_path} DIRECTORY)
#
#    if(header_dir)
#        file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/FCT/${header_dir})
#    endif()
#
#    configure_file(${header_file} ${CMAKE_CURRENT_SOURCE_DIR}/include/FCT/${relative_path} COPYONLY)
#endforeach()

target_include_directories(FCT PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

message(STATUS "Headers copied to: ${CMAKE_CURRENT_BINARY_DIR}/include")
message(STATUS "Total headers copied: ${CMAKE_LIST_LENGTH} ALL_HEADER_FILES")

function(target_link_fct_libraries target_name)
    target_link_libraries(${target_name} FCT)

    if (FCT_ANDROID)
        target_link_libraries(${target_name}
                -uJava_com_example_androidtutorial_FCTMainActivity_CPPEntry
                android
                log)
    endif()
endfunction()

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    target_precompile_headers(FCT PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h"
    )
endif()

#target_link_libraries(FCT PUBLIC
#    harfbuzz::harfbuzz
#    harfbuzz::harfbuzz-subset
#    Freetype::Freetype
#    ${Boost_LIBRARIES}
#    ${BULLET_LIBRARIES}
#)
