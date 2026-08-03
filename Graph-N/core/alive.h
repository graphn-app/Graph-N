#pragma once

using StillAliveCallback = std::function<bool()>;

class IsAlive
{
public:
    IsAlive() { m_alive = std::make_shared<bool>(true); }
    ~IsAlive() noexcept { dead(); }

    NO_COPY_MOVE(IsAlive);

    StillAliveCallback get_alive_callback() const { return [is_alive = m_alive]() noexcept { return *is_alive; }; }

protected:
    void alive() noexcept { *m_alive = true; }
    void dead() noexcept { *m_alive = false; }

private:
    const char* m_obj_name = nullptr;
    std::shared_ptr<bool> m_alive;
};
