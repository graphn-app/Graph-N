
#pragma once

namespace platform
{

class MainRoutine;

class Textures
{
public:
    Textures() = default;
    virtual ~Textures() = 0;

    NO_COPY_MOVE(Textures);

    void set_main_routine(MainRoutine* main) { m_main = main; }

protected:
    MainRoutine* m_main = nullptr;
};

inline Textures::~Textures() = default;

} // namespace platform


