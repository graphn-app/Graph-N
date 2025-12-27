
namespace base
{

std::atomic<gid> GlobalID::m_last_id;
std::string GlobalID::m_client_id;

GlobalID::GlobalID() 
{ 
    set_id(get_next_id());
}

GlobalID::GlobalID(gid id)
{
    set_id(id);
}

GlobalID::GlobalID(const GlobalID&)
{
    set_id(get_next_id());
}

GlobalID::GlobalID(GlobalID&& other)
{
    set_id(other.m_id);
}

GlobalID& GlobalID::operator= (const GlobalID&)
{
    set_id(get_next_id());
    return *this;
}

GlobalID& GlobalID::operator= (GlobalID&& other)
{
    set_id(other.m_id);
    return *this;
}

void GlobalID::set_id(gid id) 
{ 
	m_id = id; 
	m_id_str = std::to_string(m_id); 
}

void GlobalID::set_last_id(gid id)
{
	if (id > 0 && std::to_string(id) != m_client_id)
	{
		m_last_id = id;
		Log("GID initial value: " << m_last_id);
	}
}

gid GlobalID::get_next_id()
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

//////////////////////

std::atomic<lid> LocalID::m_last_id = 0;

LocalID::LocalID()
{
    set_id(get_next_id());
}

LocalID::LocalID(lid id)
{
    set_id(id);
}

LocalID::LocalID(const LocalID&)
{
    set_id(get_next_id());
}

LocalID::LocalID(LocalID&& other)
{
    set_id(other.m_id);
}

LocalID& LocalID::operator= (const LocalID&)
{
    set_id(get_next_id());
    return *this;
}

LocalID& LocalID::operator= (LocalID&& other)
{
    set_id(other.m_id);
    return *this;
}

void LocalID::set_id(lid id)
{
	m_id = id;
	m_id_str = std::to_string(m_id);
}

lid LocalID::get_next_id()
{
	return ++m_last_id;	// starts from 1
}

//////////////////////

HashID::HashID(const char* unique)
{
    set_id(tools::str::hash_fnv1a(unique));
}

HashID::HashID(const std::string& unique)
{
    set_id(tools::str::hash_fnv1a(unique));
}

HashID::HashID(const HashID& other)
{
    *this = other;
}

HashID::HashID(HashID&& other)
{
    *this = other;
}

HashID& HashID::operator= (const HashID& other)
{
    set_id(other.m_id);
    return *this;
}

HashID& HashID::operator= (HashID&& other)
{
    *this = other;
    return *this;
}

void HashID::set_id(hid id)
{
    m_id = id;
    m_id_str = std::to_string(m_id);
}

}