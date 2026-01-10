
namespace base
{

/////////////////////////////////////////////
/////   GlobalID        /////////////////////
/////////////////////////////////////////////

std::atomic<rid> GlobalID::m_last_id;
std::string GlobalID::m_client_id;

GlobalID::GlobalID()
{
    set_id(get_next_id());
}

GlobalID::GlobalID(rid id)
{
    set_id(id);
}

GlobalID::~GlobalID() = default;

void GlobalID::set_id(rid id)
{
    m_id = id;
    m_id_str = std::to_string(m_id);
}

void GlobalID::set_last_id(rid id)
{
    if (id > 0 && std::to_string(id) != m_client_id)
    {
        m_last_id = id;
        Log("GID initial value: " << m_last_id);
    }
}

rid GlobalID::get_next_id()
{
    if (0 == m_last_id)
    {
        // should never be greater than LLONG_MAX (9'223'372'036'854'775'807)

        const auto now = std::chrono::high_resolution_clock::now();
        const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        std::mt19937_64 rng(nanos);
        const std::uniform_int_distribution<uint64_t> dist(1'123'456'789, 9'999'999'999'999);
        m_last_id = dist(rng);
        Log("GID initial value: " << m_last_id);
    }

    if (std::to_string(++m_last_id) == m_client_id)
        return get_next_id();
    return m_last_id;
}

std::string GlobalID::get_client_id()
{
    if (m_client_id.empty())
    {
        m_client_id = std::to_string(get_next_id());
        Log("ClientID initialized: " << m_last_id);
    }
    return m_client_id;
}

/////////////////////////////////////////////
/////   LocalID         /////////////////////
/////////////////////////////////////////////

std::atomic<lid> LocalID::m_last_id = 0;

LocalID::LocalID()
{
    set_id(get_next_id());
}

LocalID::LocalID(lid id)
{
    set_id(id);
}

LocalID::~LocalID() = default;

void LocalID::set_id(lid id)
{
    m_id = id;
    m_id_str = std::to_string(m_id);
}

lid LocalID::get_next_id()
{
    return ++m_last_id;	// starts from 1
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

/////////////////////////////////////////////
/////   InstanceID      /////////////////////
/////////////////////////////////////////////

std::unordered_map<lid, std::set<iid>> InstanceID::m_ids;

InstanceID::InstanceID(lid workflow_id)
    : m_workflow_id(workflow_id)
{
    m_workflow_id_str = std::to_string(m_workflow_id);
    generate_id();
//    Log("InstanceID: Wf=" << m_workflow_id << ", iid=" << m_id);
}

InstanceID::~InstanceID() = default;

void InstanceID::erase_key(lid workflow_id)
{
    m_ids.erase(workflow_id);
}

void InstanceID::generate_id()
{
    auto& ids = m_ids[m_workflow_id];
    if (ids.empty())
    {
        m_ids[m_workflow_id].insert(1);
    }
    else
    {
        auto last = *ids.rbegin();
        m_ids[m_workflow_id].insert(++last);
    }
}

void InstanceID::set_id(iid id)
{
    m_id = id;
    m_id_str = std::to_string(m_id);
    m_ids[m_workflow_id].insert(m_id);
}

}
