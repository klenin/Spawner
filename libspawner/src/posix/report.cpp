#include "report.h"

// most of this code shall be merged with windows "exception" code.

struct map_cell
{
    unsigned long key;
    const char *name;
    const char *text;
};

// Exceptions are replaced with Unix signals, just for a compatibility.
const map_cell signal_identifiers[] = {
    {signal_sighup, "SIGHUP", "hangup"},
    {signal_sigint, "SIGINT", "interrupt"},
    {signal_sigquit, "SIGQUIT", "quit"},
    {signal_sigill, "SIGILL", "illegal instruction"},
    {signal_abort, "SIGABRT", "abort"},
    {signal_sigfpe, "SIGFPE", "floating point exception"},
    {signal_sigkill, "SIGKILL", "kill"},
    {signal_sigbus, "SIGBUS", "bus error"},
    {signal_sigsegv, "SIGSEGV", "segmentation violation"},
    {signal_sigterm, "SIGTERM", "software termination signal from kill"},
    {signal_sigsys, "SIGSYS", "bad argument to system call"},
    {signal_sigxcpu, "SIGXCPU", "exceeded CPU time limit"},
    {signal_sigxfsz, "SIGXFSZ", "exceeded file size limit"},
    {signal_unregistered, nullptr, "unregistered signal number"},
};

const unsigned int process_status_descriptions_count = 7;
const unsigned int terminate_reason_descriptions_count = 7;

struct process_status_description
{
    process_status_t process_status;
    const char *name;
};

const process_status_description process_status_descriptions[] = {
    {process_still_active,        "PROCESS_STILL_ACTIVE"},
    {process_suspended,           "PROCESS_SUSPENDED"},
    {process_finished_normal,     "PROCESS_FINISHED_NORMAL"},
    {process_finished_abnormally, "PROCESS_FINISHED_ABNORMALLY"},
    {process_finished_terminated, "PROCESS_FINISHED_TERMINATED"},
    {process_not_started,         "PROCESS_NOT_STARTED"},
    {process_spawner_crash,       "PROCESS_SPAWNER_CRASH"},
};

struct terminate_reason_description
{
    terminate_reason_t terminate_reason;
    const char *name;
};

const terminate_reason_description terminate_reason_descriptions[] = {
    {terminate_reason_not_terminated,           "ExitProcess"},//TERMINATE_REASON_NOT_TERMINATED"},
    {terminate_reason_none,                     "<none>"},//TERMINATE_REASON_NOT_TERMINATED"},
    {terminate_reason_abnormal_exit_process,    "AbnormalExitProcess"},//TERMINATE_REASON_NOT_TERMINATED"},
    {terminate_reason_time_limit,               "TimeLimitExceeded"},//"TERMINATE_REASON_TIME_LIMIT"},
    {terminate_reason_write_limit,              "WriteLimitExceeded"},//"TERMINATE_REASON_WRITE_LIMIT"},
    {terminate_reason_memory_limit,             "MemoryLimitExceeded"},//"TERMINATE_REASON_MEMORY_LIMIT"},
    {terminate_reason_user_time_limit,          "TimeLimitExceeded"},//"TERMINATE_REASON_USER_TIME_LIMIT"},
    {terminate_reason_load_ratio_limit,         "IdleTimeLimitExceeded"},//"TERMINATE_REASON_LOAD_RATIO_LIMIT"},
    {terminate_reason_debug_event,              "DebugEvent"},//"TERMINATE_REASON_DEBUG_EVENT"},
    {terminate_reason_created_process,          "ProcessesCountLimitExceeded"},
	{terminate_reason_not_terminated,			NULL}
};

unsigned int get_signal_index(signal_t signal)
{
	for (unsigned int k = 0; k < sizeof(signal_identifiers)/sizeof(map_cell); k++)
		if (signal_identifiers[k].key == signal)
			return k;

	// signal is unregistered in the database, pick an "unregistered"
	signal = signal_unregistered;
	for (unsigned int k = 0; k < sizeof(signal_identifiers)/sizeof(map_cell); k++)
		if (signal_identifiers[k].key == signal)
			return k;

	// unreach.
	return 0;
}

const char *get_signal_name(unsigned int index)
{
    return signal_identifiers[index].name;
}

const char * get_signal_name(signal_t signal)
{
    unsigned int index = get_signal_index(signal);
    return signal_identifiers[index].name;
}

const char *get_signal_text(unsigned int index)
{
    return signal_identifiers[index].text;
}

const char *get_signal_text(signal_t signal)
{
    unsigned int index = get_signal_index(signal);
    return signal_identifiers[index].text;
}

std::string get_signal_info(signal_t signal, std::string format)
{
	std::string res = format;
	unsigned int index = get_signal_index(signal);

	const char *name_p;
	name_p = get_signal_name(index);
	std::string name = (name_p==nullptr)?std::to_string((int)signal):name_p;

	size_t pos = 0;
	while ((pos = res.find("%n")) != std::string::npos)
		res.replace(pos, 2, name);
	while ((pos = res.find("%t")) != std::string::npos)
		res.replace(pos, 2, get_signal_text(index));
    //std::replace(res.begin(), res.end(), string("%n"), string(get_signal_name(index)));
    //std::replace(res.begin(), res.end(), string("%t"), string(get_signal_text(index)));
	return res;
}

std::string get_status_text(process_status_t process_status)
{
    for (unsigned int i = 0; i < process_status_descriptions_count; i++)
        if (process_status_descriptions[i].process_status == process_status)
            return process_status_descriptions[i].name;
    return process_status_descriptions[0].name;
}
std::string get_terminate_reason(terminate_reason_t terminate_reason)
{
    int i = 0;
    while (terminate_reason_descriptions[i].name) {
        if (terminate_reason_descriptions[i].terminate_reason == terminate_reason) {
            return terminate_reason_descriptions[i].name;
        }
        i++;
    }
    return terminate_reason_descriptions[0].name;
}

