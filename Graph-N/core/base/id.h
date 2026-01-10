#pragma once

namespace base
{

/////////////////////////////////////////////
/////   GlobalID        /////////////////////
/////////////////////////////////////////////

// TODO: needed ?

class GlobalID
{
public:
    GlobalID();
    GlobalID(rid id);
    virtual ~GlobalID() = 0;

    NO_COPY_MOVE(GlobalID);

    rid get_id() const { return m_id; }
    const std::string& get_id_str() const { return m_id_str; }
    void set_id(rid id);

    static void set_last_id(rid id);
    static rid get_next_id();
    static std::string get_client_id();

private:
    static std::atomic<rid> m_last_id;
    static std::string m_client_id;
    rid m_id = 0;
    std::string m_id_str;
};

/////////////////////////////////////////////
/////   LocalID         /////////////////////
/////////////////////////////////////////////

struct LocalID
{
    LocalID();
    LocalID(lid id);
    virtual ~LocalID() = 0;

    NO_COPY_MOVE(LocalID);

    lid get_id() const { return m_id; }
    const std::string& get_id_str() const { return m_id_str; }
    void set_id(lid id);

    static lid get_next_id();

private:
    static std::atomic<lid> m_last_id;
    lid m_id = 0;
    std::string m_id_str;
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

private:
    void set_id(hid id);

private:
    hid m_id = 0;
    std::string m_id_str;
};

/////////////////////////////////////////////
/////   InstanceID      /////////////////////
/////////////////////////////////////////////

// TODO: needed ?

struct InstanceID
{
    InstanceID(lid workflow_id);
    virtual ~InstanceID() = 0;

    NO_COPY_MOVE(InstanceID);

    static void erase_key(lid workflow_id);

protected:
    iid get_id() const { return m_id; }
    lid get_workflow_id() const { return m_workflow_id; }

    const std::string& get_id_str() const { return m_id_str; }
    const std::string& get_workflow_id_str() const { return m_workflow_id_str; }

    void set_id(iid id);

private:
    void generate_id();

private:
    static std::unordered_map<lid, std::set<iid>> m_ids;
    lid m_workflow_id = 0;
    iid m_id = 0;
    std::string m_id_str;
    std::string m_workflow_id_str;
};

}
