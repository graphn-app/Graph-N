#pragma once

using StillAliveCallback = std::function<bool()>;

class IsAlive
{
public:
    IsAlive() { m_alive = std::make_shared<bool>(true); }
    ~IsAlive() { dead(); }

    NO_COPY_MOVE(IsAlive);

    inline StillAliveCallback get_alive_callback() const { return [is_alive = m_alive]() { return *is_alive; }; }

protected:
    inline void alive() { *m_alive = true; }
    inline void dead() { *m_alive = false; }

private:
    const char* m_obj_name = nullptr;
    std::shared_ptr<bool> m_alive;
};
