#pragma once

namespace base
{

class GlobalID
{
public:
	GlobalID();
	GlobalID(gid id);
	virtual ~GlobalID() = 0 {};

	GlobalID(const GlobalID&);
	GlobalID(GlobalID&&);
	GlobalID& operator= (const GlobalID&);
	GlobalID& operator= (GlobalID&&);

	gid get_id() const { return m_id; }
	const std::string& get_id_str() const { return m_sz_id; }
	void set_id(gid id);

	static void set_last_id(gid id);
	static gid get_next_id();
	static std::string get_client_id();

private:
	static std::atomic<gid> m_last_id;
	static std::string m_client_id;
	gid m_id = 0;
	std::string m_sz_id;
};

struct LocalID
{
	LocalID();
	LocalID(lid id);
	virtual ~LocalID() = 0 {};

    LocalID(const LocalID&);
    LocalID(LocalID&&);
    LocalID& operator= (const LocalID&);
    LocalID& operator= (LocalID&&);

	lid get_id() const { return m_id; }
	const std::string& get_id_str() const { return m_sz_id; }
	void set_id(lid id);

	static lid get_next_id();

private:
	static std::atomic<lid> m_last_id;
	lid m_id = 0;
	std::string m_sz_id;
};

}