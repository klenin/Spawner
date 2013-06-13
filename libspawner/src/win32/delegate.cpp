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
    //also parse all this shit trollolo
    //program = "ping";
    options.push_argument_front(program);
    std::string session = "--session=";
    session += options.session.hash();
    options.push_argument_front(session);

    if (options.use_cmd) {
        options.push_argument_front("--cmd");
    }
    //piped redirect in fact is not required(//guess in child process)
//    create_restrictions();
    /*HANDLE job_object = CreateJobObject(NULL, NULL);
    AssignProcessToJobObject(job_object, GetCurrentProcess());
    DWORD le = GetLastError();*/

    secure_runner::create_process();

    /*JOBOBJECT_EXTENDED_LIMIT_INFORMATION extended_limit_information;
    memset(&extended_limit_information, 0, sizeof(extended_limit_information));
    extended_limit_information.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_BREAKAWAY_OK | JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;

    if (!SetInformationJobObject(job_object, JobObjectExtendedLimitInformation, &extended_limit_information, sizeof(extended_limit_information)))
        le = GetLastError();*/
    
}



delegate_instance_runner::delegate_instance_runner(const std::string &program, const options_class &options, const restrictions_class &restrictions):
    secure_runner(program, options, restrictions){
    set_pipe(STD_INPUT_PIPE, new input_pipe_class(options.session_id, "stdin"));
    set_pipe(STD_OUTPUT_PIPE, new output_pipe_class(options.session_id, "stdout"));
    set_pipe(STD_ERROR_PIPE, new output_pipe_class(options.session_id, "stderr"));
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

void delegate_instance_runner::requisites() {
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