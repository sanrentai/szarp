#ifndef __LOCATIONS_MANAGER_H__
#define __LOCATIONS_MANAGER_H__

#include <unordered_map>

#include "location.h"

#include "net/connection.h"

#include "data/vars.h"

class LocationsMgr {
public:
	LocationsMgr( const std::string& base );
	virtual ~LocationsMgr();

	void on_new_connection( Connection* conn );
	void on_disconnected  ( Connection* conn );

private:
	std::unordered_map<Connection*,Location*> locations;

	std::string base;
};

#endif /* end of include guard: __LOCATIONS_MANAGER_H__ */
