
#pragma once

namespace mngr
{

class Manager
{
public:
    Manager() = default;
    virtual ~Manager() = 0;

    NO_COPY_MOVE(Manager);

    virtual bool init() { return true; };
    virtual void free() {};
    virtual void stop_all_threads() { LogF; }
};

inline Manager::~Manager() = default;

} // namespace mngr
