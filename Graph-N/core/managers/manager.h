#pragma once

namespace mngr
{

class Manager
{
public:
    Manager() noexcept = default;
    virtual ~Manager() noexcept = 0;

    NO_COPY_MOVE(Manager);

    virtual bool init() { return true; };
    virtual void stop_and_cleanup() {}
};

inline Manager::~Manager() noexcept = default;

} // namespace mngr
