/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "core/workflow/workflow.h"
#include "core/workflow/node.h"
#include "core/managers/workflows_manager.h"
#include "imgui_node_editor_edtype.h"

namespace data
{

template <typename TBaseObj, typename TInstFlags, typename TObjFlags>
class Instance
{
public:
    Instance(TBaseObj* obj, wid workflow_id, iid instance_id) noexcept
        : m_object(obj)
        , m_wid(workflow_id)
        , m_iid(instance_id)
    {
        if (m_object != nullptr)
            m_object_flags = &m_object->flags;
    }
    virtual ~Instance() noexcept = default;

    virtual void init() {}

    const ObjectFlags<TInstFlags>& flags() const noexcept { return m_flags; }
    ObjectFlags<TInstFlags>& flags() noexcept { return m_flags; }

    const ObjectFlags<TObjFlags>& obj_flags() const noexcept
    {
        if (m_object_flags != nullptr) [[likely]]
            return *m_object_flags;
        LogError("Obj flags should not be called on empty object");
        static ObjectFlags<TObjFlags> flags;
        flags.clear();
        return flags;
    }
    ObjectFlags<TObjFlags>& obj_flags() noexcept { return cast::remove_const_ref<ObjectFlags<TObjFlags>>(std::as_const(*this).obj_flags()); }

    iid get_instance_id() const noexcept { return m_iid; }
    wid get_workflow_id() const noexcept { return m_wid; }

    void set_instance_id(iid id)
    {
        m_iid = id;
        auto wf = g::pWfsManager->access(m_wid);
        wf->set_last_child_id(id);
        wf->test_duplicates();
    }
    void set_workflow_id(wid id) noexcept { m_wid = id; }

    TBaseObj* get_object() const noexcept { return m_object; }
    bool has_object() const noexcept { return m_object != nullptr; }

    virtual void set_object(TBaseObj* obj)
    {
        m_object = obj;
        m_object_flags = nullptr;
        if (m_object != nullptr)
            m_object_flags = &m_object->flags;
    }

    EdType& ed() noexcept { return m_ed; }
    const EdType& ed() const noexcept { return m_ed; }
    virtual uint64_t reset_state() noexcept { const uint64_t id = m_ed.id(); m_ed.reset(); return id; }

protected:
    wid m_wid = 0;
    iid m_iid = 0;

private:
    TBaseObj* m_object = nullptr;     // base node/pin
    EdType m_ed;
    ObjectFlags<TInstFlags> m_flags;
    ObjectFlags<TObjFlags>* m_object_flags = nullptr;
};

} // namespace data
