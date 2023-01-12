// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xutl.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xmetrics/xmetrics.h"
#include "xstatistic/xstatistic.h"

NS_BEG2(top, xstatistic)

#ifdef CACHE_SIZE_STATISTIC
xstatistic_t * xstatistic_hub_t::_static_statistic = nullptr;

xstatistic_t* xstatistic_hub_t::instance() {
    if(_static_statistic)
        return _static_statistic;

    xdbg("xstatistic_hub_t::instance");
    _static_statistic = new xstatistic_t();
    return _static_statistic;
}

// xstatistic_obj_face_t::xstatistic_obj_face_t() {
//     m_create_time = base::xtime_utl::gmttime_ms();
//     xstatistic_hub_t::instance()->add_object(this);
// }

xstatistic_obj_face_t::xstatistic_obj_face_t(enum_statistic_class_type type) {
    m_type = type;
    xdbg("xstatistic_obj_face_t::xstatistic_obj_face_t this:%p,type:%d", this, type);
    m_create_time = base::xtime_utl::gmttime_ms();
    xstatistic_hub_t::instance()->add_object(this);
}

xstatistic_obj_face_t::xstatistic_obj_face_t(const xstatistic_obj_face_t & obj) {
    xdbg("xstatistic_obj_face_t::xstatistic_obj_face_t copy this:%p from:%p,type:%d", this, &obj, obj.m_type);
    // std::cout << "xstatistic_obj_face_t copy this : " << this << " obj : " << &obj << std::endl;
    m_create_time = obj.m_create_time;
    m_type = obj.m_type;
    m_size = obj.m_size;
    xstatistic_hub_t::instance()->add_object(this);
}

// xstatistic_obj_face_t::~xstatistic_obj_face_t() {
//     xdbg("xstatistic_obj_face_t::~xstatistic_obj_face_t copy this:%p,type:%d", this, m_type);
//     // std::cout << "~xstatistic_obj_face_t this : " << this << std::endl;
//     xstatistic_hub_t::instance()->del_object(this);
// }

void xstatistic_obj_face_t::statistic_del() {
    xdbg("xstatistic_obj_face_t::statistic_del copy this:%p,type:%d", this, m_type);
    xstatistic_hub_t::instance()->del_object(this);
}

const int32_t xstatistic_obj_face_t::get_object_size() const {
    if (m_size == 0) {
        try {
            m_size = get_object_size_real();
        }catch (std::exception & eh) {
            std::cerr << "xstatistic_obj_face_t::get_object_size failed. exception: " << eh.what() << std::endl;
            xerror("xstatistic_obj_face_t::get_object_size failed with exception: %s,this:%p, type:%d", eh.what(), this, m_type);
        }
    }
    return m_size;
}

void xobject_statistic_base_t::add_object(xstatistic_obj_face_t * object) {
    std::lock_guard<std::mutex> lck(m_mutex);
    refresh_inner(object->create_time());
#ifdef USE_MULTISET_ONLY
    m_not_calc_object_set.insert(object);
#else
    auto iter = m_not_calc_object_set.insert(object);
    m_not_calc_object_map[object] = iter;
#endif
    // xdbg("xobject_statistic_base_t::add_object object:%p,create_time:%llu", object, object->create_time());
    xdbg("xobject_statistic_base_t::add_object object:%p, type:%d", object, object->get_class_type());
}

void xobject_statistic_base_t::del_object(xstatistic_obj_face_t * object) {
    std::lock_guard<std::mutex> lck(m_mutex);
#ifdef USE_MULTISET_ONLY
    auto ret = m_not_calc_object_set.equal_range(object);
    for (auto it = ret.first; it != ret.second; it++) {
        xdbg("xobject_statistic_base_t::del_object it:%p, object:%p, type:%d", (*it), object, object->get_class_type());
        if ((*it) == object) {
            xdbg("xobject_statistic_base_t::del_object erase object:%p, type:%d", object, object->get_class_type());
            m_not_calc_object_set.erase(it);
            refresh_inner(base::xtime_utl::gmttime_ms());
            return;
        }
    }

    update_metrics(object->get_class_type(), -1, -object->get_object_size());
    refresh_inner(base::xtime_utl::gmttime_ms());
#else
    auto it_map = m_not_calc_object_map.find(object);
    if (it_map != m_not_calc_object_map.end()) {
        xdbg("xobject_statistic_base_t::del_object from map object:%p, type:%d", object, object->get_class_type());
        m_not_calc_object_set.erase(it_map->second);
        m_not_calc_object_map.erase(it_map);
    } else {
        xdbg("xobject_statistic_base_t::del_object del size object:%p, type:%d", object, object->get_class_type());
        update_metrics(object->get_class_type(), -1, -object->get_object_size());
    }
    
    refresh_inner(base::xtime_utl::gmttime_ms());
#endif
}

void xobject_statistic_base_t::refresh() {
    auto now = base::xtime_utl::gmttime_ms();
    std::lock_guard<std::mutex> lck(m_mutex);
    refresh_inner(now);
}

void xobject_statistic_base_t::refresh_inner(int64_t now) {
    for (auto iter = m_not_calc_object_set.begin(); iter != m_not_calc_object_set.end();) {
        xdbg("xobject_statistic_base_t::refresh_inner object:%p, type:%d, create time:%llu, m_delay_time:%llu, now:%llu",
             (*iter),
             (*iter)->get_class_type(),
             (*iter)->create_time(),
             m_delay_time,
             now);
        if ((*iter)->create_time() + m_delay_time <= now) {
            auto size = (*iter)->get_object_size();
            update_metrics((*iter)->get_class_type(), 1, size);
            xdbg("xobject_statistic_base_t::refresh_inner object:%p, size:%d, type:%d", (*iter), size, (*iter)->get_class_type());
#ifndef USE_MULTISET_ONLY
            m_not_calc_object_map.erase(*iter);
#endif
            iter = m_not_calc_object_set.erase(iter);
        } else {
            return;
        }
    }
}

void xobject_statistic_base_t::update_metrics(int32_t type, int32_t change_num, int32_t change_size) {
    int num_metrics_tag = metrics::statistic_tx_v2_num + type - enum_statistic_begin;
    int size_metrics_tag = metrics::statistic_tx_v2_size + type - enum_statistic_begin;
    XMETRICS_GAUGE((metrics::E_SIMPLE_METRICS_TAG)num_metrics_tag, change_num);
    XMETRICS_GAUGE((metrics::E_SIMPLE_METRICS_TAG)size_metrics_tag, change_size);
}

xstatistic_t::xstatistic_t() {
    int64_t delay_time = XGET_CONFIG(calculate_size_delay_time);
    xdbg("xstatistic_t::xstatistic_t delay_time:%d", delay_time);
    for (uint32_t i = enum_statistic_begin; i < enum_statistic_max; i++) {
        m_object_statistic_vec.push_back(new xobject_statistic_base_t(delay_time));
    }
}

void xstatistic_t::add_object(xstatistic_obj_face_t * object) {
    uint32_t idx = (uint32_t)object->get_class_type() - 1;
    m_object_statistic_vec[idx]->add_object(object);
}

void xstatistic_t::del_object(xstatistic_obj_face_t * object) {
    uint32_t idx = (uint32_t)object->get_class_type() - 1;
    m_object_statistic_vec[idx]->del_object(object);
}

void xstatistic_t::refresh() {
    for (auto & object_statistic : m_object_statistic_vec) {
        xdbg("xstatistic_t::refresh");
        object_statistic->refresh();
    }
}
#endif

NS_END2
