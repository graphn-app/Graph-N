
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
    }
    while (true);
    return ret;
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

hid HashID::make_hash(const std::string& unique)
{
    return tools::str::hash_fnv1a(unique);
}

HashID::HashID(const char* unique)
{
    set_id(make_hash(unique));
//    Log("HashID: " << unique << "=" << m_id);
}

HashID::HashID(const std::string& unique)
{
    set_id(make_hash(unique));
//    Log("HashID: " << unique << "=" << m_id);
}

HashID::~HashID() = default;

void HashID::set_id(hid id)
{
    m_id = id;
    m_id_str = std::to_string(m_id);
}

} // namespace base
