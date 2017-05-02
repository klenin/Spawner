#include <session.h>
#include <memory.h>
#include <iomanip>
#include "platform.h"

//session_manager_class session_manager_class::session_manager_instance;
session_class session_class::base_session;

session_class::session_class() {
    md5_init(&md5_state);
    (*this) << get_spawner_pid();
}

session_class::session_class(const session_class &session) {
    md5_init(&md5_state);
    memcpy(&md5_state, &session.md5_state, sizeof(md5_state_t));
}

std::string session_class::hash() const {
    md5_state_t md5_state_tmp;
    md5_byte_t digest[16];
    std::ostringstream result;

    memcpy(&md5_state_tmp, &md5_state, sizeof(md5_state_t));
    md5_finish(&md5_state_tmp, digest);

    for (int i = 0; i < 16; ++i) {
        result << std::setfill('0') << std::setw(2) << std::hex << (int)digest[i];
    }
    return result.str();
}

//template<typename T>
//session_class& session_class::operator<< (const T &val)
