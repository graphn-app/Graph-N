#pragma once

namespace base
{

/////////////////////////////////////////////
/////   GlobalID        /////////////////////
/////////////////////////////////////////////

class GlobalID final
{
public:
    static std::string uuid();
    static std::string client_id();

private:
    GlobalID();
    ~GlobalID() = default;

    NO_COPY_MOVE(GlobalID);

    static GlobalID& instance()
    {
        static GlobalID inst;
        return inst;
    }

private:
    std::mt19937_64 m_gen;
    std::mutex m_lock;
    std::once_flag m_flag;
    std::string m_client_id;
};

/////////////////////////////////////////////
/////   HashID          /////////////////////
/////////////////////////////////////////////

struct HashID
{
    HashID(const char* unique);
    HashID(const std::string& unique);
    virtual ~HashID() = 0;

    NO_COPY_MOVE(HashID);

    hid get_id() const { return m_id; }
    const std::string& get_id_str() const { return m_id_str; }

    static hid make_hash(const std::string& unique);

private:
    void set_id(hid id);

private:
    hid m_id = 0;
    std::string m_id_str;
};

}
