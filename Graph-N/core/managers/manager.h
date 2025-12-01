
#pragma once

namespace mngr
{

class Manager
{
public:
	Manager() = default;
	virtual ~Manager() = 0 {};

	virtual bool init() = 0;
	virtual void free() = 0;
};

} // namespace mngr