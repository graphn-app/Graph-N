
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

std::string GlobalID::uuid()
{
    uint64_t part1 = 0, part2 = 0;
    {
        auto& self = instance();
        std::lock_guard<std::mutex> lock(self.m_lock);
        std::uniform_int_distribution<uint64_t> dist;
        part1 = dist(self.m_gen);
        part2 = dist(self.m_gen);
    }

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

HashID::HashID(const char* unique)
{
    set_id(tools::str::hash_fnv1a(unique));
//    Log("HashID: " << unique << "=" << m_id);
}

HashID::HashID(const std::string& unique)
{
    set_id(tools::str::hash_fnv1a(unique));
//    Log("HashID: " << unique << "=" << m_id);
}

HashID::~HashID() = default;

void HashID::set_id(hid id)
{
    m_id = id;
    m_id_str = std::to_string(m_id);
}

} // namespace base
