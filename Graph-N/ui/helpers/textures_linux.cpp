/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#ifdef __linux__
#define GL_GLEXT_PROTOTYPES
#include "GLFW/glfw3.h"
#include "GL/gl.h"
#include "ui/helpers/textures_linux.h"

namespace platform
{

ImTextureIDPtr Textures_Linux::upload_to_gpu(const byte* image_data, uint32_t width, uint32_t height, bool generate_mips)
{
    LogRAssert(image_data, "Invalid input", nullptr);

    GLuint tex = 0;
    glGenTextures(1, &tex);
    if (0 == tex)
    {
        LogWarnS("glGenTextures failed");
        return nullptr;
    }

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, generate_mips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLint>(width), static_cast<GLint>(height), 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

    if (generate_mips)
        glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    // The OpenGL texture name (a GLuint) is passed to ImGui as the ImTextureID
    // (void*). Its lifetime is owned by the returned shared_ptr.
    return ImTextureIDPtr(
        reinterpret_cast<void*>(static_cast<uintptr_t>(tex)),
        [tex](void*) noexcept { glDeleteTextures(1, &tex); });
}

bool Textures_Linux::update_gpu_texture(ImTextureIDPtr texture, const byte* image_data, uint32_t width, uint32_t height, bool generate_mips)
{
    LogRAssert(image_data && texture, "Invalid input", false);

    const GLuint tex = static_cast<GLuint>(reinterpret_cast<uintptr_t>(texture.get()));

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLint>(width), static_cast<GLint>(height), GL_RGBA, GL_UNSIGNED_BYTE, image_data);

    if (generate_mips)
        glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

} // namespace platform

#endif // #ifdef __linux__
