#ifndef _SPAWNER_SESSION_H_
#define _SPAWNER_SESSION_H_

#include <string>
#include <sstream>

#include <cstdint>

#include <lib/md5_cpp11.h>

class session_class {
private:
    md5_cpp11::builder md5_state;
    session_class();
public:
    session_class(const session_class &session);
    static session_class base_session;
    std::string hash() const;
    operator std::string() const {return hash();}
    template<typename T>
    session_class& operator<< (const T &val) {
        std::ostringstream stringstream_tmp;
        std::string str;

        stringstream_tmp << val;
        str = stringstream_tmp.str();
        md5_state.update(reinterpret_cast<const std::uint8_t*>(str.c_str()), str.length());
        return *this;
    }

};

#endif//_SPAWNER_SESSION_H_
