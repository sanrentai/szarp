#ifndef __SZ4_LOAD_CURRENT_FILE_H__
#define __SZ4_LOAD_CURRENT_FILE_H__
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
namespace sz4
{

bool load_current_file(const boost::filesystem::wpath& path, second_time_t& t, std::vector<unsigned char> &data);
bool load_current_file(const boost::filesystem::wpath& path, nanosecond_time_t& t, std::vector<unsigned char> &data);

}

#endif