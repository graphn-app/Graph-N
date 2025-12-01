#pragma once

namespace base
{

class GlobalID
{
public:
	GlobalID();
	GlobalID(gid id);
	virtual ~GlobalID() = 0 {};

	GlobalID(const GlobalID&) = delete;
	GlobalID(GlobalID&&) = delete;
	GlobalID& operator= (const GlobalID&) = delete;
	GlobalID& operator= (GlobalID&&) = delete;

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

class LocalID
{
public:
	LocalID();
	LocalID(lid id);
	virtual ~LocalID() = 0 {};

	LocalID(const LocalID&) = delete;
	LocalID(LocalID&&) = delete;
	LocalID& operator= (const LocalID&) = delete;
	LocalID& operator= (LocalID&&) = delete;

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