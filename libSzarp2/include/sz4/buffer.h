/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#ifndef __SZ4_BUFFER_H__
#define __SZ4_BUFFER_H__

#include "config.h"

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include <boost/preprocessor/seq/for_each_product.hpp>
#include <boost/preprocessor/seq/elem.hpp>

#include "szbase/szbparamobserver.h"
#include "szbase/szbparammonitor.h"

#include "szarp_base_common/defines.h"
#include "sz4/defs.h"
#include "sz4/types.h"
#include "sz4/time.h"
#include "sz4/block.h"
#include "sz4/path.h"
#include "sz4/param_observer.h"
#include "sz4/load_file_locked.h"

namespace sz4 {

template<class types> class base_templ;

#define SZ4_BASE_DATA_TYPE_SEQ (short)(int)(float)(double)
#define SZ4_BASE_TIME_TYPE_SEQ (second_time_t)(nanosecond_time_t)


class generic_param_entry : public SzbParamObserver {
protected:
	boost::recursive_mutex m_lock;
	TParam* m_param;

	std::list<generic_param_entry*> m_referring_params;
	std::list<generic_param_entry*> m_referred_params;
	std::list<param_observer*> m_observers;
public:
	generic_param_entry(TParam* param) : m_param(param) {}
	TParam* get_param() const { return m_param; }

	/* Macro below generates methods
	 virtual void get_weighted_sum(time_type start_time, time_type end_type, weighted_sum<time_type, value_type>() = 0;
	 for each possible combination of value and time types
	*/
#	define SZ4_GENERATE_ABSTRACT_GET_WSUM(r, seq) 	\
	virtual void get_weighted_sum(const BOOST_PP_SEQ_ELEM(1, seq)& start, const BOOST_PP_SEQ_ELEM(1, seq)& end, SZARP_PROBE_TYPE probe_type, weighted_sum<BOOST_PP_SEQ_ELEM(0, seq), BOOST_PP_SEQ_ELEM(1, seq)>& wsum) = 0;

	BOOST_PP_SEQ_FOR_EACH_PRODUCT_R(1, SZ4_GENERATE_ABSTRACT_GET_WSUM, (SZ4_BASE_DATA_TYPE_SEQ)(SZ4_BASE_TIME_TYPE_SEQ))

#	undef SZ4_GENERATE_ABSTRACT_GET_WSUM
	

	virtual second_time_t search_data_right(const second_time_t& start, const second_time_t& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) = 0;
	virtual second_time_t search_data_left(const second_time_t& start, const second_time_t& end,  SZARP_PROBE_TYPE probe_type,const search_condition& condition) = 0;
	virtual nanosecond_time_t search_data_right(const nanosecond_time_t& start, const nanosecond_time_t& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) = 0;
	virtual nanosecond_time_t search_data_left(const nanosecond_time_t& start, const nanosecond_time_t& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) = 0;

	virtual void get_first_time(nanosecond_time_t& t) = 0;
	virtual void get_first_time(second_time_t& t) = 0;

	virtual void get_last_time(nanosecond_time_t& t) = 0;
	virtual void get_last_time(second_time_t& t) = 0;

	virtual void register_at_monitor(SzbParamMonitor* monitor) = 0;
	virtual void deregister_from_monitor(SzbParamMonitor* monitor) = 0;
	void param_data_changed(TParam*, const std::string& path);
	virtual void handle_param_data_changed(TParam*, const std::string& path) {};


	void add_refferring_param(generic_param_entry* param_entry);
	void remove_refferring_param(generic_param_entry* param_entry);

	void add_refferred_param(generic_param_entry *param_entry);
	virtual void refferred_param_removed(generic_param_entry* param_entry);

	void register_observer(param_observer *observer);
	void deregister_observer(param_observer *observer);

	virtual ~generic_param_entry();
};

template<class V, class T> class block_entry {
protected:
	concrete_block<V, T> m_block;
	bool m_needs_refresh;
	const boost::filesystem::wpath m_block_path;
public:
	block_entry(const T& start_time, block_cache* cache) :
		m_block(start_time, cache), m_needs_refresh(true) {}

	void get_weighted_sum(const T& start, const T& end, weighted_sum<V, T>& wsum) {
		m_block.get_weighted_sum(start, end, wsum);
	}

	T search_data_right(const T& start, const T& end, const search_condition& condition) {
		return m_block.search_data_right(start, end, condition);
	}

	T search_data_left(const T& start, const T& end, const search_condition& condition) {
		return m_block.search_data_left(start, end, condition);
	}

	void set_needs_refresh() {
		m_needs_refresh = true;
	}

	concrete_block<V, T>& block() { return m_block; }
};

namespace param_buffer_type_converion_helper {

template<class IV, class IT, class OV, class OT> class helper {
	weighted_sum<IV, IT> m_temp;
	weighted_sum<OV, OT>& m_sum;
public:
	helper(weighted_sum<OV, OT>& sum) : m_sum(sum) {}
	weighted_sum<IV, IT>& sum() { return m_temp; }
	void convert() {
		m_sum.sum() = m_temp.sum();
		m_sum.weight() = m_temp.weight();
		m_sum.no_data_weight() = m_temp.no_data_weight();
		m_sum.set_fixed(m_temp.fixed());
		m_sum.refferred_blocks().swap(m_temp.refferred_blocks());
	}
};

template<class V, class T> class helper<V, T, V, T> {
	weighted_sum<V, T>& m_sum;
public:
	helper(weighted_sum<V, T>& sum) : m_sum(sum) {}
	weighted_sum<V, T>& sum() { return m_sum; }
	void convert() { }
};

}

template<template <typename DT, typename TT, typename BT> class PT, class V, class T, class BT> class param_entry_in_buffer : public generic_param_entry {
	PT<V, T, BT> m_entry;
	
	template<class RV, class RT> void get_weighted_sum_templ(const T& start, const T& end, SZARP_PROBE_TYPE probe_type, weighted_sum<RV, RT>& sum)  {
		param_buffer_type_converion_helper::helper<V, T, RV, RT> helper(sum);
		m_entry.get_weighted_sum_impl(start, end, probe_type, helper.sum());
		helper.convert();
	}
public:
	param_entry_in_buffer(base_templ<BT>* _base, TParam* param, const boost::filesystem::wpath& base_dir) :
		generic_param_entry(param), m_entry(_base, param, base_dir / param->GetSzbaseName())
	{ }

#	define SZ4_GENERATE_GET_WSUM(r, seq) 	\
	void get_weighted_sum(const BOOST_PP_SEQ_ELEM(1, seq)& start, const BOOST_PP_SEQ_ELEM(1, seq)& end, SZARP_PROBE_TYPE probe_type, weighted_sum<BOOST_PP_SEQ_ELEM(0, seq), BOOST_PP_SEQ_ELEM(1, seq)>& wsum) {	  \
		get_weighted_sum_templ(start, end, probe_type, wsum);	\
	}

	BOOST_PP_SEQ_FOR_EACH_PRODUCT_R(1, SZ4_GENERATE_GET_WSUM, (SZ4_BASE_DATA_TYPE_SEQ)(SZ4_BASE_TIME_TYPE_SEQ))
#	undef SZ4_GENERATE_GET_WSUM

	second_time_t search_data_right(const second_time_t& start, const second_time_t& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		return search_data_right_templ(start, end, probe_type, condition);
	}

	second_time_t search_data_left(const second_time_t& start, const second_time_t& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		return search_data_left_templ(start, end, probe_type, condition);
	}

	nanosecond_time_t search_data_right(const nanosecond_time_t& start, const nanosecond_time_t& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		return search_data_right_templ(start, end, probe_type, condition);
	}

	nanosecond_time_t search_data_left(const nanosecond_time_t& start, const nanosecond_time_t& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		return search_data_left_templ(start, end, probe_type, condition);
	}

	template<class RT> RT search_data_right_templ(const RT& start, const RT &end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		return RT(m_entry.search_data_right_impl(T(start), round_up<RT, T>()(end), probe_type, condition));
	}

	template<class RT> RT search_data_left_templ(const RT& start, const RT &end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		return RT(m_entry.search_data_left_impl(T(start), round_up<RT, T>()(end), probe_type, condition));
	}

	template<class RT> void get_first_time_templ(RT& t) {
		t = RT(m_entry.get_first_time());
	}

	void get_first_time(nanosecond_time_t& t) {
		get_first_time_templ(t);
	}

	void get_first_time(second_time_t& t) {
		get_first_time_templ(t);
	}

	template<class RT> void get_last_time_templ(RT& t) {
		t = RT(m_entry.get_last_time());
	}

	void get_last_time(nanosecond_time_t& t) {
		get_last_time_templ(t);
	}	

	void get_last_time(second_time_t& t) {
		get_last_time_templ(t);
	}

	void register_at_monitor(SzbParamMonitor* monitor) {
		m_entry.register_at_monitor(this, monitor);
	}

	void deregister_from_monitor(SzbParamMonitor* monitor) {
		m_entry.deregister_from_monitor(this, monitor);
	}

	void handle_param_data_changed(TParam* param, const std::string& path) {
		m_entry.param_data_changed(param, path);
	}
			
	PT<V, T, base_templ<BT> >& get_contained_entry() {
		return m_entry;
	}

	virtual ~param_entry_in_buffer() {
	}
};

template<class types> class buffer_templ {
	typedef typename types::ipk_container_type ipk_container_type;
	base_templ<types>* m_base;
	SzbParamMonitor* m_param_monitor;
	ipk_container_type* m_ipk_container;
	boost::filesystem::wpath m_buffer_directory;
	std::vector<generic_param_entry*> m_param_ents;

	generic_param_entry* m_heart_beat_entry;

	void prepare_param(TParam* param);
public:
	buffer_templ(base_templ<types>* _base, SzbParamMonitor* param_monitor, ipk_container_type* ipk_container, const std::wstring& prefix, const std::wstring& buffer_directory)
			: m_base(_base), m_param_monitor(param_monitor), m_ipk_container(ipk_container), m_buffer_directory(buffer_directory) {
	
		TParam* heart_beat_param = m_ipk_container->GetParam(prefix + L":Status:Meaner4:Heartbeat");
		if (heart_beat_param)
			m_heart_beat_entry = get_param_entry(m_ipk_container->GetParam(prefix + L":Status:Meaner4:Heartbeat"));
		else
			m_heart_beat_entry = NULL;
	}

	generic_param_entry* get_param_entry(TParam* param) {
		if (m_param_ents.size() <= param->GetParamId())
			m_param_ents.resize(param->GetParamId() + 1, NULL);

		generic_param_entry* entry = m_param_ents[param->GetParamId()];
		if (entry == NULL) {
			entry = m_param_ents[param->GetParamId()] = create_param_entry(param);
			entry->register_at_monitor(m_param_monitor);
		}
		return entry;
	}

	template<class T, class V> void get_weighted_sum(TParam* param, const T& start, const T &end, SZARP_PROBE_TYPE probe_type, weighted_sum<V, T> &wsum) {
		get_param_entry(param)->get_weighted_sum(start, end, probe_type, wsum);
	}

	template<class T> T search_data_right(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition &condition) {
		return get_param_entry(param)->search_data_right(start, end, probe_type, condition);
	}

	template<class T> T search_data_left(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition &condition) {
		return get_param_entry(param)->search_data_left(start, end, probe_type, condition);
	}

	generic_param_entry* create_param_entry(TParam* param);

	void get_first_time(nanosecond_time_t& t) {
		m_heart_beat_entry->get_first_time(t);
	}

	void get_first_time(second_time_t& t) {
		m_heart_beat_entry->get_first_time(t);
	}

	void get_last_time(nanosecond_time_t& t) {
		m_heart_beat_entry->get_last_time(t);
	}

	void get_last_time(second_time_t& t) {
		m_heart_beat_entry->get_last_time(t);
	}

	void remove_param(TParam* param);

	~buffer_templ() {
		for (std::vector<generic_param_entry*>::iterator i = m_param_ents.begin(); i != m_param_ents.end(); i++) {
			if (*i)
				(*i)->deregister_from_monitor(m_param_monitor);
			delete *i;
		}
	}
};

typedef buffer_templ<base_types> buffer;

}

#endif