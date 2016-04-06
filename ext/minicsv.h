// The MIT License (MIT)
// Minimalistic CSV Streams 1.6
// Copyright (C) 2014, by Wong Shao Voon (shaovoon@yahoo.com)
//
// http://opensource.org/licenses/MIT
//
// version 1.2   : make use of make_shared
// version 1.3   : fixed: to when reading the last line and it does not have linefeed
//                 added: skip_1st_line and skip_line functions to ifstream class
// version 1.4   : Removed the use of smart ptr.
// version 1.5   : Performance increase on writing without flushing every line.
// version 1.6   : Add string streams
// version 1.7   : You MUST specify the escape/unescape string when calling set_delimiter. Option to surround/trim string with quotes
// version 1.7.1 : Add stream operator overload usage in example.cpp
//                 Disable the surround/trim quote on text by default
// version 1.7.2 : Stream operator overload for const char*
// version 1.7.3 : Add num_of_delimiter method to ifstream and istringstream
//                 Fix g++ compilation errors
// version 1.7.4 : Add get_rest_of_line
// version 1.7.5 : Add terminate_on_blank_line variable. Set to false if your file format has blank lines in between.
// version 1.7.6 : Ignore delimiters within quotes during reading when enable_trim_quote_on_str is true;
// version 1.7.7 : Fixed multiple symbol linkage errors

//#define USE_BOOST_LEXICAL_CAST

#ifndef MiniCSV_H
	#define MiniCSV_H

#include <string>
#include <sstream>
#include <fstream>

#ifdef USE_BOOST_LEXICAL_CAST
#	include <boost/lexical_cast.hpp>
#endif

#define NEWLINE '\n'

namespace csv
{
	inline std::string const & replace(std::string & src, std::string const & to_find, std::string const & to_replace)
	{
		size_t pos = 0;
		while (std::string::npos != pos)
		{
			pos = src.find(to_find, pos);

			if (std::string::npos != pos)
			{
				src.erase(pos, to_find.size());
				src.insert(pos, to_replace);
				pos += to_replace.size();
			}
		}

		return src;
	}

	inline std::string trim_right(const std::string& str, const std::string& trimChars)
	{
		std::string result = "";
		size_t endpos = str.find_last_not_of(trimChars);
		if (std::string::npos != endpos)
		{
			result = str.substr(0, endpos + 1);
		}
		else
			result = str;

		return result;
	}

	inline std::string trim_left(const std::string& str, const std::string& trimChars)
	{
		std::string result = "";

		size_t startpos = str.find_first_not_of(trimChars);
		if (std::string::npos != startpos)
		{
			result = str.substr(startpos);
		}
		else
			result = str;

		return result;
	}

	inline std::string trim(const std::string& str, const std::string& trimChars)
	{
		return trim_left(trim_right(str, trimChars), trimChars);
	}

	class ifstream
	{
	public:
		ifstream() : str(""), pos(0), delimiter(","), unescape_str("##"), trim_quote_on_str(false), trim_quote('\"'), terminate_on_blank_line(true)
		{
		}
		ifstream(const char * file)
		{
			open(file);
		}
		void open(const char * file)
		{
			init();
			istm.open(file, std::ios_base::in);
		}
		void init()
		{
			str = "";
			pos = 0;
			delimiter = ',';
			unescape_str = "##";
			trim_quote_on_str = false;
			trim_quote = '\"';
			terminate_on_blank_line = true;
		}
		void close()
		{
			istm.close();
		}
		bool is_open()
		{
			return istm.is_open();
		}
		void enable_trim_quote_on_str(bool enable, char quote)
		{
			trim_quote_on_str = enable;
			trim_quote = quote;
		}
		// eof is replaced by read_line
		//bool eof() const
		void set_delimiter(char delimiter_, std::string const & unescape_str_)
		{
			delimiter = delimiter_;
			unescape_str = unescape_str_;
		}
		std::string const &  get_delimiter() const
		{
			return delimiter;
		}
		std::string const &  get_unescape_str() const
		{
			return unescape_str;
		}
		void skip_line()
		{
			if(!istm.eof())
			{
				std::getline(istm, str);
				pos = 0;
			}
		}
		bool read_line()
		{
			this->str = "";
			while(!istm.eof())
			{
				std::getline(istm, this->str);
				pos = 0;

				if (this->str.empty())
				{
					if (terminate_on_blank_line)
						break;
					else
						continue;
				}

				return true;
			}
			return false;
		}
		std::string get_delimited_str()
		{
			std::string str = "";
			char ch = '\0';
			bool within_quote = false;
			do
			{
				if(pos>=this->str.size())
				{
					this->str = "";

					return unescape(str);
				}

				ch = this->str[pos];
				if (trim_quote_on_str)
				{
					if (within_quote == false && ch == trim_quote && ((pos > 0 && this->str[pos - 1] == delimiter[0]) || pos == 0))
						within_quote = true;
					else if (within_quote && ch == trim_quote)
						within_quote = false;
				}

				++(pos);

				if (ch == delimiter[0] && within_quote == false)
					break;
				if (ch == '\r' || ch == '\n')
					break;

				str += ch;
			}
			while(true);

			return unescape(str);
		}
		std::string unescape(std::string & src)
		{
			src = unescape_str.empty() ? src : replace(src, unescape_str, delimiter);
			return trim_quote_on_str ? trim(src, std::string(1, trim_quote)) : src;
		}
		size_t num_of_delimiter() const
		{
			if (delimiter.size() == 0)
				return 0;

			size_t cnt = 0;
			for (size_t i = 0; i < str.size(); ++i)
			{
				if (str[i] == delimiter[0])
					++cnt;
			}
			return cnt;
		}
		std::string get_rest_of_line() const
		{
			return str.substr(pos);
		}
		const std::string& get_line() const
		{
			return str;
		}
		void enable_terminate_on_blank_line(bool enable)
		{
			terminate_on_blank_line = enable;
		}
		bool is_terminate_on_blank_line() const
		{
			return terminate_on_blank_line;
		}

	private:
		std::ifstream istm;
		std::string str;
		size_t pos;
		std::string delimiter;
		std::string unescape_str;
		bool trim_quote_on_str;
		char trim_quote;
		bool terminate_on_blank_line;
	};

	class ofstream
	{
	public:

		ofstream() : after_newline(true), delimiter(","), escape_str("##"), surround_quote_on_str(false), surround_quote('\"')
		{
		}
		ofstream(const char * file)
		{
			open(file);
		}
		void open(const char * file)
		{
			init();
			ostm.open(file, std::ios_base::out);
		}
		void init()
		{
			after_newline = true; 
			delimiter = ',';
			escape_str = "##";
			surround_quote_on_str = false;
			surround_quote = '\"';
		}
		void flush()
		{
			ostm.flush();
		}
		void close()
		{
			ostm.close();
		}
		bool is_open()
		{
			return ostm.is_open();
		}
		void enable_surround_quote_on_str(bool enable, char quote)
		{
			surround_quote_on_str = enable;
			surround_quote = quote;
		}
		void set_delimiter(char delimiter_, std::string const & escape_str_)
		{
			delimiter = delimiter_;
			escape_str = escape_str_;
		}
		std::string const &  get_delimiter() const
		{
			return delimiter;
		}
		std::string const &  get_escape_str() const
		{
			return escape_str;
		}
		void set_after_newline(bool after_newline_)
		{
			after_newline = after_newline_;
		}
		bool get_after_newline() const
		{
			return after_newline;
		}
		std::ofstream& get_ofstream()
		{
			return ostm;
		}
		void escape_and_output(std::string src)
		{
			ostm << ((escape_str.empty()) ? src : replace(src, delimiter, escape_str));
		}
		void escape_str_and_output(std::string src)
		{
			src = ((escape_str.empty()) ? src : replace(src, delimiter, escape_str));
			if (surround_quote_on_str)
			{
				ostm << surround_quote << src << surround_quote;
			}
			else
			{
				ostm << src;
			}
		}
	private:
		std::ofstream ostm;
		bool after_newline;
		std::string delimiter;
		std::string escape_str;
		bool surround_quote_on_str;
		char surround_quote;
	};


} // ns csv

template<typename T>
csv::ifstream& operator >> (csv::ifstream& istm, T& val)
{
	std::string str = istm.get_delimited_str();
	
#ifdef USE_BOOST_LEXICAL_CAST
	val = boost::lexical_cast<T>(str);
#else
	std::istringstream is(str);
	is >> val;
#endif

	return istm;
}
template<>
inline csv::ifstream& operator >> (csv::ifstream& istm, std::string& val)
{
	val = istm.get_delimited_str();

	return istm;
}

template<typename T>
csv::ofstream& operator << (csv::ofstream& ostm, const T& val)
{
	if(!ostm.get_after_newline())
		ostm.get_ofstream() << ostm.get_delimiter();

	std::ostringstream os_temp;

	os_temp << val;

	ostm.escape_and_output(os_temp.str());

	ostm.set_after_newline(false);

	return ostm;
}

template<typename T>
csv::ofstream& operator << (csv::ofstream& ostm, const T* val)
{
	if (!ostm.get_after_newline())
		ostm.get_ofstream() << ostm.get_delimiter();

	std::ostringstream os_temp;

	os_temp << *val;

	ostm.escape_and_output(os_temp.str());

	ostm.set_after_newline(false);

	return ostm;
}

template<>
inline csv::ofstream& operator << (csv::ofstream& ostm, const std::string& val)
{
	if (!ostm.get_after_newline())
		ostm.get_ofstream() << ostm.get_delimiter();

	std::string temp = val;
	ostm.escape_str_and_output(temp);

	ostm.set_after_newline(false);

	return ostm;
}
template<>
inline csv::ofstream& operator << (csv::ofstream& ostm, const char& val)
{
	if(val==NEWLINE)
	{
		ostm.get_ofstream() << NEWLINE;

		ostm.set_after_newline(true);
	}
	else
	{
		std::ostringstream os_temp;

		os_temp << val;

		ostm.escape_and_output(os_temp.str());
	}

	return ostm;
}
template<>
inline csv::ofstream& operator << (csv::ofstream& ostm, const char* val)
{
	const std::string temp = val;

	ostm << temp;

	return ostm;
}

namespace csv
{

class istringstream
{
public:
	istringstream(const char * text)
		: str("")
		, pos(0)
		, delimiter(",")
		, unescape_str("##")
		, trim_quote_on_str(false)
		, trim_quote('\"')
		, terminate_on_blank_line(true)
	{
		istm.str(text);
	}
	void enable_trim_quote_on_str(bool enable, char quote)
	{
		trim_quote_on_str = enable;
		trim_quote = quote;
	}
	void set_delimiter(char delimiter_, std::string const & unescape_str_)
	{
		delimiter = delimiter_;
		unescape_str = unescape_str_;
	}
	std::string const & get_delimiter() const
	{
		return delimiter;
	}
	std::string const &  get_unescape_str() const
	{
		return unescape_str;
	}
	void skip_line()
	{
		std::getline(istm, str);
		pos = 0;
	}
	bool read_line()
	{
		this->str = "";
		while (!istm.eof())
		{
			std::getline(istm, this->str);
			pos = 0;

			if (this->str.empty())
			{
				if (terminate_on_blank_line)
					break;
				else
					continue;
			}

			return true;
		}
		return false;
		return false;
	}
	std::string get_delimited_str()
	{
		std::string str = "";
		char ch = '\0';
		bool within_quote = false;
		do
		{
			if (pos >= this->str.size())
			{
				this->str = "";

				return unescape(str);
			}

			ch = this->str[pos];
			if (trim_quote_on_str)
			{
				if (within_quote == false && ch == trim_quote && ((pos > 0 && this->str[pos - 1] == delimiter[0]) || pos == 0))
					within_quote = true;
				else if (within_quote && ch == trim_quote)
					within_quote = false;
			}

			++(pos);

			if (ch == delimiter[0] && within_quote == false)
				break;
			if (ch == '\r' || ch == '\n')
				break;

			str += ch;
		} while (true);

		return unescape(str);
	}

	std::string unescape(std::string & src)
	{
		src = unescape_str.empty() ? src : replace(src, unescape_str, delimiter);
		return trim_quote_on_str ? trim(src, std::string(1, trim_quote)) : src;
	}

	size_t num_of_delimiter() const
	{
		if (delimiter.size() == 0)
			return 0;

		size_t cnt = 0;
		for (size_t i = 0; i < str.size(); ++i)
		{
			if (str[i] == delimiter[0])
				++cnt;
		}
		return cnt;
	}
	std::string get_rest_of_line() const
	{
		return str.substr(pos);
	}
	const std::string& get_line() const
	{
		return str;
	}
	void enable_terminate_on_blank_line(bool enable)
	{
		terminate_on_blank_line = enable;
	}
	bool is_terminate_on_blank_line() const
	{
		return terminate_on_blank_line;
	}

private:
	std::istringstream istm;
	std::string str;
	size_t pos;
	std::string delimiter;
	std::string unescape_str;
	bool trim_quote_on_str;
	char trim_quote;
	bool terminate_on_blank_line;
};

class ostringstream
{
public:

	ostringstream() : after_newline(true), delimiter(","), escape_str("##"), surround_quote_on_str(false), surround_quote('\"')
	{
	}
	void enable_surround_quote_on_str(bool enable, char quote)
	{
		surround_quote_on_str = enable;
		surround_quote = quote;
	}
	void set_delimiter(char delimiter_, std::string const & escape_str_)
	{
		delimiter = delimiter_;
		escape_str = escape_str_;
	}
	std::string const & get_delimiter() const
	{
		return delimiter;
	}
	std::string const &  get_escape_str() const
	{
		return escape_str;
	}
	void set_after_newline(bool after_newline_)
	{
		after_newline = after_newline_;
	}
	bool get_after_newline() const
	{
		return after_newline;
	}
	std::ostringstream& get_ostringstream()
	{
		return ostm;
	}
	std::string get_text()
	{
		return ostm.str();
	}
	void escape_and_output(std::string src)
	{
		ostm << ((escape_str.empty()) ? src : replace(src, delimiter, escape_str));
	}
	void escape_str_and_output(std::string src)
	{
		src = ((escape_str.empty()) ? src : replace(src, delimiter, escape_str));
		if (surround_quote_on_str)
		{
			ostm << surround_quote << src << surround_quote;
		}
		else
		{
			ostm << src;
		}
	}

private:
	std::ostringstream ostm;
	bool after_newline;
	std::string delimiter;
	std::string escape_str;
	bool surround_quote_on_str;
	char surround_quote;
};


} // ns csv

template<typename T>
csv::istringstream& operator >> (csv::istringstream& istm, T& val)
{
	std::string str = istm.get_delimited_str();

#ifdef USE_BOOST_LEXICAL_CAST
	val = boost::lexical_cast<T>(str);
#else
	std::istringstream is(str);
	is >> val;
#endif

	return istm;
}
template<>
inline csv::istringstream& operator >> (csv::istringstream& istm, std::string& val)
{
	val = istm.get_delimited_str();

	return istm;
}

template<typename T>
csv::ostringstream& operator << (csv::ostringstream& ostm, const T& val)
{
	if (!ostm.get_after_newline())
		ostm.get_ostringstream() << ostm.get_delimiter();

	std::ostringstream os_temp;

	os_temp << val;

	ostm.escape_and_output(os_temp.str());

	ostm.set_after_newline(false);

	return ostm;
}
template<typename T>
csv::ostringstream& operator << (csv::ostringstream& ostm, const T* val)
{
	if (!ostm.get_after_newline())
		ostm.get_ostringstream() << ostm.get_delimiter();

	std::ostringstream os_temp;

	os_temp << val;

	ostm.escape_and_output(os_temp.str());

	ostm.set_after_newline(false);

	return ostm;
}
template<>
inline csv::ostringstream& operator << (csv::ostringstream& ostm, const std::string& val)
{
	if (!ostm.get_after_newline())
		ostm.get_ostringstream() << ostm.get_delimiter();

	std::string temp = val;
	ostm.escape_str_and_output(temp);

	ostm.set_after_newline(false);

	return ostm;
}

template<>
inline csv::ostringstream& operator << (csv::ostringstream& ostm, const char& val)
{
	if (val == NEWLINE)
	{
		ostm.get_ostringstream() << NEWLINE;

		ostm.set_after_newline(true);
	}
	else
	{
		std::ostringstream os_temp;

		os_temp << val;

		ostm.escape_and_output(os_temp.str());
	}

	return ostm;
}
template<>
inline csv::ostringstream& operator << (csv::ostringstream& ostm, const char* val)
{
	const std::string temp = val;

	ostm << temp;

	return ostm;
}


#endif // MiniCSV_H