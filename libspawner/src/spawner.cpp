#include <spawner.h>
/*spawner spawner::_instance;

spawner::spawner()
{
#ifdef  WIN32
    debug_thread = CreateThread(NULL, 0, debug, this, 0, NULL);
#endif//WIN32
}
spawner::~spawner()
{
#ifdef  WIN32
    //for (process_map::iterator it = processes.p)
    CloseHandleSafe(debug_thread);
#endif//WIN32
}

thread_return_t spawner::debug(thread_param_t param)
{
/*    spawner *self = (spawner*)param;
#ifdef  WIN32
    while (true)
    {
        DEBUG_EVENT debug_event = {0};
        BOOL res = WaitForDebugEvent(&debug_event, INFINITE);
        DWORD le = GetLastError();
        process_map::iterator found = self->processes.find(debug_event.dwProcessId);
        if (found != self->processes.end())
        {
            (*found).second->set_terminate_reason(terminate_reason_debug_event);
        }
        ContinueDebugEvent(debug_event.dwProcessId,
            debug_event.dwThreadId,
            DBG_CONTINUE);
    }
#endif//WIN32/
    return 0;
}

spawner &spawner::instance()
{
    return _instance;
}

//************************************************************************/
//* spawn_process(program_name, restrictions, options)                   */
//*                                                                      */
//*          initiates process named <program_name> with provided        */
//*      restrictions <restrictions> and options <options>. also adds    */
//*      this process to internal map for future manipulation            */
//*                                                                      */
//*          returns pointer to process object ready to run              */
//************************************************************************/
/*CProcess *spawner::spawn_process(const std::string &program_name, const CRestrictions &restrictions, const COptions &options)
{
    CProcess *new_process = new CProcess(program_name);
    new_process->set_options(options);
    new_process->set_restrictions(restrictions);
    new_process->prepare();
    processes[new_process->get_process_id()] = new_process;
    return new_process;
}
*/