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
/*
 * meaner3 - daemon writing data to base in SzarpBase format
 * SZARP
 
 * Pawe� Pa�ucha pawel@praterm.com.pl
 
 * $Id$
 */

#ifndef __TSAVEPARAM_H__
#define __TSAVEPARAM_H__

#include "boost/filesystem/path.hpp"
#include "boost/filesystem/convenience.hpp"


#include "tstatus.h"
#include "szarp_config.h"

namespace fs = boost::filesystem;

/** Class represents parameter saved to base. */
class TSaveParam {
	public:
		/** Creates object corresponding to given IPK parameter.
		 * @param p corresponding IPK parameter */
		TSaveParam(TParam* p);
		/** Creates object for param with given name. Used for status
		 * parameters.
		 * @param name name of parameters
		 * @param convert decide if name should be converted with wchar2szb
		 */

		TSaveParam(const std::wstring& name , bool convert = true );
		~TSaveParam();
		/** Write param to base.
		 * @param directory base directory
		 * @param t save time
		 * @param data pointer to buffer with data to write
		 * @param data_count number of elements in buffer to save
		 * @param status if not NULL status object is used for counting
		 * not-null parameters
		 * @param overwrite 0 if is overwriting of existing data 
		 * is not allowed, 1 otherwise
		 * @param force_nodata 1 to force writing no-data values to file
		 * @param probe_length of probe in seconds, default value for meaner, 10 for prober
		 * @return 0 on success, 1 on error
		 */
		int Write(const fs::wpath& directory, 
				time_t t, 
				short int* data, 
				size_t data_count,
				TStatus *status,
				int overwrite, 
				int force_nodata, 
				time_t probe_length);
		/** Write param to base - version that saves one element.
		 * @see Write
		 */
		int Write(const fs::wpath& directory, 
				time_t t, 
				short int data, 
				TStatus *status,
				int overwrite = 0,
				int force_nodata = 0,
				time_t probe_length = SZBASE_DATA_SPAN);
		/**
		 * @brief performs @see Write operation but tries minimise opening/closing
		 *        of data cache file
		 * 
		 * @param force_same_file forcing using same file every write. Creating new file path
		 * and checking every time string-based if file has changed was pretty heavy. Defaults
		 * to false to backward compatibility.
		 */
		int WriteBuffered(const fs::wpath& directory, time_t t, short int* data, size_t data_count, TStatus *status, 
		                int overwrite, int force_nodata, time_t probe_length , bool force_same_file = false );
		/** Write 10-seconds probe to disk cache.
		 * @see Write
		 */
		int WriteProbes(const fs::wpath& directory, time_t t, short int* data, size_t data_count);
		/** Closes currenly open file desctiptor*/
		void CloseFile();

		/** 
		 * @brief Creating file path based on necessary informations
		 * 
		 * @param dir file base directory
		 * @param name converted parameter name 
		 * @param year probe year
		 * @param month probe month
		 * @param probe_length probe type
		 */
		void CreateFilePath( const fs::wpath& dir , const std::wstring& name , int year , int month , time_t probe_length );

		/** 
		 * @brief Parameter name getter
		 * 
		 * @return return converted parameter name.
		 */
		const std::wstring& GetName() { return cname; }
	protected:
		int fd;			/**< Currently open data file desriptor*/
		std::wstring cname;	/**< Encoded name of parameter */
		std::wstring m_path;      /**< full path to file */
		std::wstring last_path; /**< Last file data was written to*/
};

#endif

