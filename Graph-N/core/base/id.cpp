
namespace base
{

std::atomic<gid> GlobalID::m_last_id;
std::string GlobalID::m_client_id;

GlobalID::GlobalID() 
{ 
	m_id = get_next_id(); 
	m_sz_id = std::to_string(m_id);
}

GlobalID::GlobalID(gid id)
{
	m_id = id;
	m_sz_id = std::to_string(m_id);
}

void GlobalID::set_id(gid id) 
{ 
	m_id = id; 
	m_sz_id = std::to_string(m_id); 
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

std::atomic<lid> LocalID::m_last_id;

LocalID::LocalID()
{
	m_id = get_next_id();
	m_sz_id = std::to_string(m_id);
}

LocalID::LocalID(lid id)
{
	m_id = id;
	m_sz_id = std::to_string(m_id);
}

void LocalID::set_id(lid id)
{
	m_id = id;
	m_sz_id = std::to_string(m_id);
}

lid LocalID::get_next_id()
{
	return ++m_last_id;	// start from 1
}

}