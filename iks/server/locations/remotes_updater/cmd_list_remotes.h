#ifndef __REMOTES_CMD_LIST_REMOTES_H__
#define __REMOTES_CMD_LIST_REMOTES_H__

#include <sstream>

#include <liblog.h>

#include "locations/command.h"
#include "locations/locations_list.h"
#include "locations/proxy/proxy.h"

#include "utils/ptree.h"

class CmdListRemotesSnd : public Command {
public:
	CmdListRemotesSnd( 
		LocationsList& locs ,
		const std::string& name , const std::string& draw_name ,
		const std::string& address , unsigned port )
		: locs(locs) , name(name) , draw_name(draw_name) , address(address) , port(port)
	{
		set_next( std::bind(
				&CmdListRemotesSnd::parse_command ,
				this,std::placeholders::_1) );
	}

	virtual ~CmdListRemotesSnd() {}

	virtual to_send send_str()
	{	return to_send( "" ); }

	void parse_command( const std::string& line )
	{
		namespace bp = boost::property_tree;

		std::stringstream ss(line);
		bp::ptree json;

		try {
			bp::json_parser::read_json( ss , json );

			for( auto itr=json.begin() ; itr!=json.end() ; ++itr )
				locs.register_location<ProxyLoc>
					( str( boost::format("%s:%s") % name % itr->first ) ,
					  draw_name.empty() ?
						  itr->second.get<std::string>("name") :
						  str( boost::format("%s - %s") %
							  draw_name %
							  itr->second.get<std::string>("name") ) ,
					  itr->second.get<std::string>("type") ,
					  address, port );

		} catch( const bp::ptree_error& e ) {
			sz_log(1 , "CmdListRemotesSnd: Received invalid message (%s): %s", line.c_str(), e.what());
			return;
		}
	}

protected:
	LocationsList& locs;
	const std::string& name;
	const std::string& draw_name;
	const std::string& address;
	unsigned port;

};

#endif /* end of include guard: __REMOTES_CMD_LIST_REMOTES_H__ */

