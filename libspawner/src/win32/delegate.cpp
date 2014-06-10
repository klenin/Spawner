#include <inc/delegate.h>
#include <inc/error.h>

const char *SPAWNER_PROGRAM = "sp.exe";


delegate_runner::delegate_runner(const std::string &program, const options_class &options, const restrictions_class &restrictions):
    secure_runner(program, options, restrictions)
{
    force_program = SPAWNER_PROGRAM;
}

/*void delegate_runner::requisites() {
/*    if (ResumeThread(process_info.hThread) == (DWORD)-1)
    {
        raise_error(*this, "ResumeThread");
        return;
    }
    check_thread = CreateThread(NULL, 0, check_limits_proc, this, 0, NULL);
}*/

void delegate_runner::set_allow_breakaway(bool allow) {
}

bool delegate_runner::apply_restrictions() {
    return true;
}

void delegate_runner::create_process() {
    options.use_cmd = true;

    //program = "ping";
    options.push_argument_front(program);
    std::string session = "--session=";
    session += options.session.hash();
    options.push_argument_front(session);

    if (options.use_cmd) {
        options.push_argument_front("--cmd");
    }
    options.push_argument_front("--out=std");
    options.push_argument_front("--err=std");
    options.push_argument_front("--in=std");
    options.push_argument_front("-hr");
    options.push_argument_front("--legacy=sp00");

    secure_runner::create_process();

}



delegate_instance_runner::delegate_instance_runner(const std::string &program, const options_class &options, const restrictions_class &restrictions):
    secure_runner(program, options, restrictions){
}

bool delegate_instance_runner::create_restrictions() {
    std::string name = "Local\\";
    name += options.session_id;
#ifdef OPEN_JOB_OBJECT_DYNAMIC_LOAD
    HINSTANCE hDLL_1 = LoadLibrary("kernel32.dll");
    OpenJobObjectA = (OPEN_JOB_OBJECT)GetProcAddress(hDLL_1, "OpenJobObjectA");
    FreeLibrary(hDLL_1);
    //load_open_job_object();
#endif
    hJob = OpenJobObject(JOB_OBJECT_ASSIGN_PROCESS, FALSE, name.c_str());
    //raise_error
    return true;
}

void delegate_instance_runner::requisites_() {
    apply_restrictions();
    if (ResumeThread(process_info.hThread) == (DWORD)-1)
    {
        raise_error(*this, "ResumeThread");
        return;
    }
}

void delegate_instance_runner::wait() {
    //while (1) Sleep(1000);
}