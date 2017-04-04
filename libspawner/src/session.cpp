#include <session.h>
#include <memory.h>
#include <iomanip>
#include "platform.h"

//session_manager_class session_manager_class::session_manager_instance;
session_class session_class::base_session;

session_class::session_class() {
    (*this) << get_spawner_pid();
}

session_class::session_class(const session_class &session)
    : md5_state(session.md5_state)
{}

std::string session_class::hash() const {
    using namespace md5_cpp11;
    return to_hex_string(builder(md5_state).finalize());
}

//template<typename T>
//session_class& session_class::operator<< (const T &val)
