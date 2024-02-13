#ifndef _UTIL_FOPEN_HPP
#define _UTIL_FOPEN_HPP

#include <string>

size_t get_line_number();
void file_open_read(const std::string& filename);
void file_open_write(const std::string& filename);
bool read_line(std::string& line);
void write_line(std::string&& line);
void file_close_read();
void file_close_write();

#endif
