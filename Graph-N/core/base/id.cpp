#include "core/base/id.h"
#include "imgui_widget_limits.h"

namespace base
{

/////////////////////////////////////////////
/////   GlobalID        /////////////////////
/////////////////////////////////////////////

GlobalID::GlobalID()
    : m_gen(std::random_device{}())
{
}

std::string GlobalID::client_id()
{
    auto& self = instance();
    std::call_once(self.m_flag, [&self]()
    {
        std::lock_guard<std::mutex> lock(self.m_lock);
        std::uniform_int_distribution<uint64_t> dist;
        const uint64_t id = dist(self.m_gen);
        self.m_client_id = std::to_string(id);
    });

    return self.m_client_id;
}

uint64_t GlobalID::random()
{
    static std::set<uint64_t> all_generated;

    auto& self = instance();
    std::lock_guard<std::mutex> lock(self.m_lock);
    std::uniform_int_distribution<uint64_t> dist;

    uint64_t ret = 0;
    do
    {
        ret = dist(self.m_gen);
        if (all_generated.insert(ret).second)
            break;
        LogWarnS("Duplicate random value");
    }
    while (true);
    return ret;
}

int64_t GlobalID::random_widget(int64_t min, int64_t max)
{
    if (min > max) std::swap(min, max);
    if (min < IM_S64_MIN_WIDGET) min = IM_S64_MIN_WIDGET;
    if (max > IM_S64_MAX_WIDGET) max = IM_S64_MAX_WIDGET;

    auto& self = instance();
    std::lock_guard<std::mutex> lock(self.m_lock);
    std::uniform_int_distribution<int64_t> dist(min, max);
    return dist(self.m_gen);
}

int64_t GlobalID::random_seed()
{
    return random_widget(0, IM_S64_MAX_WIDGET);
}

std::string GlobalID::uuid()
{
    uint64_t part1 = random();
    uint64_t part2 = random();

    // version
    part1 = (part1 & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    // variant
    part2 = (part2 & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    // UUID : 8-4-4-4-12 
    ss << std::setw(8) << (part1 >> 32) << "-";
    ss << std::setw(4) << ((part1 >> 16) & 0xFFFF) << "-";
    ss << std::setw(4) << (part1 & 0xFFFF) << "-";
    ss << std::setw(4) << (part2 >> 48) << "-";
    ss << std::setw(12) << (part2 & 0xFFFFFFFFFFFFULL);

    return ss.str();
}

/////////////////////////////////////////////
/////   HashID          /////////////////////
/////////////////////////////////////////////

HashID::HashID(const std::string& unique) noexcept
{
    m_hash = make_hash(unique);
}

HashID::~HashID() noexcept = default;

hid HashID::make_hash(const std::string& unique)
{
    return tools::str::hash_stable(unique);
}

} // namespace base
