
#pragma once

namespace mngr
{

class Manager
{
public:
	Manager() = default;
	virtual ~Manager() = 0 {};

	virtual bool init() { return true; };
	virtual void free() {};
};

} // namespace mngr