
#pragma once

namespace platform
{

class MainRoutine;

class Textures
{
public:
	Textures() = default;
	virtual ~Textures() = 0 {};

	void set_main_routine(MainRoutine* main) { m_main = main; }

protected:
	MainRoutine* m_main = nullptr;
};

} // namespace platform


