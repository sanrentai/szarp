#ifndef ARGSLINEPARSER__H__
#define ARGSLINEPARSER__H__

#include <boost/optional.hpp>
#include "libpar.h"
#include "cmdlineparser.h"
#include <type_traits>

template <typename RT>
typename std::enable_if<
     std::is_signed<RT>::value,
RT>::type cast_numeric_val(const std::string& v_str) {
	try {
		return boost::lexical_cast<RT>(v_str);
	} catch (...) {
		return std::stoll(v_str, nullptr, 0);
	}
}

template <typename RT>
typename std::enable_if<
	std::is_unsigned<RT>::value,
RT>::type cast_numeric_val(const std::string& v_str) {
	try {
		return boost::lexical_cast<RT>(v_str);
	} catch (...) {
		return std::stoull(v_str, nullptr, 0);
	}
}

template <typename RT>
RT cast_val(const std::string& v_str) {
	return cast_numeric_val<RT>(v_str);
}


class ArgsManager {
	std::string base_arg;
	mutable bool libpar_initialized = false;

public:
	ArgsManager(const std::string& _name): name(_name), cmd(name) {}
	ArgsManager(std::string&& _name): name(_name), cmd(name) {}

	ArgsManager& operator=(ArgsManager&& other) = default; 
	ArgsManager(ArgsManager&& other) = default;

	ArgsManager(const ArgsManager&) = default;
	ArgsManager& operator=(const ArgsManager&) = default;

	~ArgsManager() {
		// Uncomment this once libpar is not used outside ArgsManager
		//libpar_done();
	}

	template <typename... As>
	void parse(int argc, const char ** argv, const As&... as) {
		cmd.parse(argc, argv, as...);
		base_arg = cmd.get("base").get_value_or("");
	}

	template <typename... As>
	void parse(int argc, char ** argv, const As&... as) {
		parse(argc, const_cast<const char **>(argv), as...);
	}

	// this should be called from main thread (not mt-safe)
	void initLibpar() const {
		if (libpar_initialized) return;

		if (!base_arg.empty()) {
			#ifndef MINGW32
				libpar_init_from_folder("/opt/szarp/"+base_arg+"/");
			#else
				libpar_init_with_filename(("%HOMEDRIVE%\\%HOMEPATH%\\.szarp\\"+base_arg+"\\config\\params.xml").c_str(), 1);
			#endif
		} else {
			#ifndef MINGW32
				const std::string SZARP_CFG = "/etc/szarp/szarp.cfg";
				libpar_init_with_filename(SZARP_CFG.c_str(), 0);
			#else
				throw std::runtime_error("Cannot initialize without base prefix");
			#endif
		}

		libpar_initialized = true;
	}

	template <typename T = std::string>
	boost::optional<T> get(const std::string& section, const std::string& arg) const;

	template <typename T = std::string>
	boost::optional <T> get(const std::string& arg) const { 
		return get<T>(name, arg);
	}

	bool has(const std::string& arg) const {
		return has(name, arg);
	}

	bool has(const std::string& section, const std::string& arg) const {
		if (cmd.count(arg)) return true;

		auto libp_arg = get_libparval<std::string>(section, arg);
		if (libp_arg) if (!libp_arg->empty()) return true;

		return false;
	}

	size_t count(const std::string& arg) const {
		return cmd.count(arg);
	}

	const CmdLineParser get_cmdlineparser() const {
		return cmd;
	}

private:
	// Getting -D... parameters from cmd line. This is for backward compatibility.
	template <typename T>
	boost::optional<T> get_cmdval(const std::string& arg) const;

	// Getting -D... parameters from cmd line. This is for backward compatibility.
	template <typename T>
	boost::optional<T> get_overriden(const std::string& section, const std::string& arg) const;

	// Getting parameters from cfg file (to refactor later on).
	template <typename T>
	boost::optional<T> get_libparval(const std::string& section, const std::string& arg) const;

	std::string name;
	CmdLineParser cmd;
};

template <typename T>
boost::optional<T> ArgsManager::get(const std::string& section, const std::string& arg) const { 
	auto cmd_parser_ret = get_cmdval<T>(arg);
	if (cmd_parser_ret) return *cmd_parser_ret;

	auto ov_ret = get_overriden<T>(section, arg);
	if (ov_ret) return *ov_ret;

	auto lp_ret = get_libparval<T>(section, arg);
	if (lp_ret) return *lp_ret;

	return boost::none;
}

template <typename T>
boost::optional<T> ArgsManager::get_cmdval(const std::string& arg) const {
	if (cmd.vm.count(arg) > 0) {
		try  {
			return *cmd.get<T>(arg);
		} catch(...) {}
	}

	return boost::none;
}

template <typename T>
boost::optional<T> ArgsManager::get_libparval(const std::string& section, const std::string& arg) const { 
	if (!libpar_initialized) return boost::none;

	char* libpar_val;
	if (section.empty()) 
		libpar_val = libpar_getpar(name.c_str(), arg.c_str(), 0);
	else
		libpar_val = libpar_getpar(section.c_str(), arg.c_str(), 0);
	
	if (libpar_val == NULL) return boost::none;

	try {
		auto str_val = std::string(libpar_val);
		free(libpar_val);
		return cast_val<T>(str_val);
	} catch(const boost::bad_lexical_cast& e) {
		return boost::none;
	}
}

template <typename T>
boost::optional<T> ArgsManager::get_overriden(const std::string& section, const std::string& arg) const { 
	if (cmd.vm.count("override") > 0) {
		auto overwritten_params = cmd.vm["override"].as<std::vector<std::string>>();
		for (auto str: overwritten_params) {
			auto eq_sign_pos = str.find_first_of("=");
			if (str.substr(0,eq_sign_pos) == arg) {
				try  {
					return cast_val<T>(str.substr(eq_sign_pos+1));
				} catch(const boost::bad_lexical_cast& e) {}
			}
		}
	}

	return boost::none;
}

template <>
bool cast_val(const std::string& v_str);

template <>
const char* cast_val(const std::string& v_str);

template <>
char* cast_val(const std::string& v_str);

template <>
const wchar_t* cast_val(const std::string& v_str);

template <>
wchar_t* cast_val(const std::string& v_str);

template <>
const std::string cast_val(const std::string& v_str);

template <>
std::string cast_val(const std::string& v_str);

template <>
const std::wstring cast_val(const std::string& v_str);

template <>
std::wstring cast_val(const std::string& v_str);

#endif
