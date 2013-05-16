#include "pipes.h"
#include <error.h>
#include <iostream>
#include <fstream>

input_buffer_class dummy_input_buffer;
output_buffer_class dummy_output_buffer;
//move this to separate function

pipe_class::pipe_class(): pipe_type(PIPE_UNDEFINED), buffer_thread(handle_default_value), 
    reading_mutex(handle_default_value) {
}
pipe_class::pipe_class(std_pipe_t pipe_type): pipe_type(pipe_type), buffer_thread(handle_default_value), 
    reading_mutex(handle_default_value) {
    create_pipe();
}
// http://summerpinn.wordpress.com/2011/03/13/child-process-output-redirection-asynchronous-named-pipe/
bool CreatePipeEx(
    const char *PipeNameBuffer,
    bool read,
    OUT LPHANDLE lpReadPipe,
    OUT LPHANDLE lpWritePipe,
    IN LPSECURITY_ATTRIBUTES lpPipeAttributes
    )
{
    //HANDLE ReadPipeHandle, WritePipeHandle;
    DWORD dwError;
    HANDLE named_pipe_handle;
    //LPTSTR PipeNameBuffer = TEXT("\\\\.\\pipe\\unique_name");
    DWORD open_mode = (read?PIPE_ACCESS_OUTBOUND:PIPE_ACCESS_INBOUND);// | FILE_FLAG_OVERLAPPED;

    HANDLE tmp_named_pipe_handle = CreateNamedPipe( PipeNameBuffer,
									  open_mode,
                                      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE |PIPE_WAIT,
                                      //PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                      1,    // Number of pipes
                                      0,    // Out buffer size
                                      0,    // In buffer size
                                      0,    // Timeout in ms
                                      lpPipeAttributes );
    //ConnectNamedPipe(named_pipe_handle, NULL);
    if (! tmp_named_pipe_handle) {
        return false;
    }

    HANDLE connected_pipe_handle = CreateFile(
                        PipeNameBuffer,
                        (read?FILE_READ_DATA:FILE_WRITE_DATA),// | SYNCHRONIZE,
                        NULL,
                        lpPipeAttributes,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,//FILE_FLAG_OVERLAPPED
                        NULL                      
                      );

    if (INVALID_HANDLE_VALUE == connected_pipe_handle) {
        dwError = GetLastError();
        CloseHandle( connected_pipe_handle );
        SetLastError(dwError);
        return false;
    }
    DuplicateHandle(GetCurrentProcess(), tmp_named_pipe_handle,
        GetCurrentProcess(), &named_pipe_handle, 0,
        FALSE, // not inherited
        DUPLICATE_SAME_ACCESS);
    CloseHandle(tmp_named_pipe_handle);
    if (!read) {
        *lpReadPipe = named_pipe_handle;
        *lpWritePipe = connected_pipe_handle;
    } else {
        *lpReadPipe = connected_pipe_handle;
        *lpWritePipe = named_pipe_handle;
    }
    return true;
}

HANDLE create_named_pipe(const char *name, const std_pipe_t &pipe_type) {
    SECURITY_ATTRIBUTES saAttr;
    HANDLE tmp_named_pipe_handle;
    HANDLE named_pipe_handle;

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL;

    DWORD open_mode = (pipe_type == PIPE_INPUT)?PIPE_ACCESS_OUTBOUND:PIPE_ACCESS_INBOUND;// | FILE_FLAG_OVERLAPPED;

    tmp_named_pipe_handle = CreateNamedPipe(name,
									  open_mode,
                                      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE |PIPE_WAIT,
                                      //PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                      1,    // Number of pipes
                                      0,    // Out buffer size
                                      0,    // In buffer size
                                      0,    // Timeout in ms
                                      &saAttr);

    if (!tmp_named_pipe_handle) {
        return tmp_named_pipe_handle;
    }

    DuplicateHandle(
        GetCurrentProcess(), tmp_named_pipe_handle,
        GetCurrentProcess(), &named_pipe_handle, 0,
        FALSE, // not inherited
        DUPLICATE_SAME_ACCESS
        );
    CloseHandle(tmp_named_pipe_handle);

    return named_pipe_handle;
}

handle_t open_named_pipe(const char *name, const std_pipe_t &pipe_type) {
    SECURITY_ATTRIBUTES saAttr;
    HANDLE connected_pipe_handle;
    DWORD dwError;

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL;
    
    connected_pipe_handle = CreateFile(
                        name,
                        (pipe_type == PIPE_INPUT?FILE_READ_DATA:FILE_WRITE_DATA),// | SYNCHRONIZE,
                        NULL,
                        &saAttr,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,//FILE_FLAG_OVERLAPPED
                        NULL                      
                      );

    if (INVALID_HANDLE_VALUE == connected_pipe_handle) {
        dwError = GetLastError();
        CloseHandle( connected_pipe_handle );
        SetLastError(dwError);
        return connected_pipe_handle;
    }
}

pipe_t pipe_class::input_pipe() { 
    if (!readPipe) {
        readPipe = open_named_pipe(name.c_str(), pipe_type);
    }
    return readPipe;
}

pipe_t pipe_class::output_pipe() {
    if (!writePipe) {
        writePipe = open_named_pipe(name.c_str(), pipe_type);
    }
    return writePipe;
}

#include <sstream>
bool pipe_class::create_pipe()
{
    readPipe = handle_default_value;
    writePipe = handle_default_value;
    buffer_thread = handle_default_value;
    reading_mutex = handle_default_value;
    state = true;
    std::ostringstream str;
    str << "\\\\.\\pipe\\mynamedpipe";
    str << rand();
    name = str.str();

    handle_t pipe_handle = create_named_pipe(name.c_str(), pipe_type);
    if (pipe_type == PIPE_INPUT) {
        writePipe = pipe_handle;
        readPipe = 0;
    } else {
        readPipe = pipe_handle;
        writePipe = 0;
    }

    /*HANDLE handle = readPipe;
    if (pipe_type == PIPE_OUTPUT) {
        handle = writePipe;
    }
    if (!SetHandleInformation(handle, HANDLE_FLAG_INHERIT, 0)) {
        raise_error(*this, "SetHandleInformation");
        return false;
    }*/
    return true;
}

void pipe_class::close_pipe()
{
    if (pipe_type & PIPE_INPUT)
        CloseHandleSafe(readPipe);
    if (pipe_type & PIPE_OUTPUT)
        CloseHandleSafe(writePipe);
    finish();
}

pipe_class::~pipe_class()
{
    close_pipe();
}

size_t pipe_class::write( void *data, size_t size )
{
    DWORD dwWritten;
    if (!WriteFile(writePipe, data, size, &dwWritten, NULL))// || dwWritten != size
    {
        raise_error(*this, "WriteFile");
        return 0;
    }
    if (!FlushFileBuffers(writePipe))
    {
        raise_error(*this, "FlushFileBuffers");
        return 0;
    }
    return dwWritten;
}

size_t pipe_class::read(void *data, size_t size)
{
    DWORD dwRead;
    if (!ReadFile(readPipe, data, size, &dwRead, NULL))
    {
        raise_error(*this, "ReadFile");
        return 0;
    }
    return dwRead;
}


bool pipe_class::bufferize()
{
    if (buffer_thread != handle_default_value)
    {
        //trying to bufferize twice
        return false;
    }        
    if (reading_mutex == handle_default_value)
    {
        reading_mutex = CreateMutex(NULL, FALSE, NULL);
    }
    return true;
}

void pipe_class::wait()
{
    if (reading_mutex == handle_default_value || buffer_thread == handle_default_value)
        return;
    WaitForSingleObject(reading_mutex, INFINITE);
}

void pipe_class::finish()
{
    if (reading_mutex != handle_default_value)
    {
        WaitForSingleObject(reading_mutex, INFINITE);
        ReleaseMutex(reading_mutex);
    }
    CloseHandleSafe(buffer_thread);
    CloseHandleSafe(reading_mutex);
}

void pipe_class::wait_for_pipe(const unsigned int &ms_time)
{
    WaitForSingleObject(buffer_thread, ms_time);
}

void pipe_class::safe_release()
{
    close_pipe();
    state = false;
}

bool pipe_class::valid()
{
    return state;
}

pipe_t pipe_class::get_pipe()
{
    return 0;
}




thread_return_t input_pipe_class::writing_buffer(thread_param_t param) {
    input_pipe_class *self = (input_pipe_class*)param;

    char buffer[BUFFER_SIZE + 1];
    size_t read_count;
    if (!self->input_buffer->readable())
        return 0;
    while (read_count = self->input_buffer->read(buffer, BUFFER_SIZE)) {
        if (!self->write(buffer, read_count))
            break;
        /*if (!self->input_buffer->readable())
            return 0;*/
    }
    return 0;
}

input_pipe_class::input_pipe_class(): pipe_class(PIPE_INPUT), input_buffer(&dummy_input_buffer){
}

input_pipe_class::input_pipe_class(input_buffer_class *input_buffer_param): 
    pipe_class(PIPE_INPUT), input_buffer(input_buffer_param){
}

bool input_pipe_class::bufferize()
{
    if (!pipe_class::bufferize())
        return false;
    buffer_thread = CreateThread(NULL, 0, writing_buffer, this, 0, NULL);
    if (!buffer_thread)
    {
        raise_error(*this, "CreateThread");
        return false;
    }
    return true;
}

pipe_t input_pipe_class::get_pipe()
{
    return input_pipe();
}


thread_return_t output_pipe_class::reading_buffer(thread_param_t param)
{
    output_pipe_class *self = (output_pipe_class*)param;
    ConnectNamedPipe(self->readPipe, 0);
    if (!self->output_buffer->writeable())
        return 0;
    for (;;)
    {
        DWORD	nAvailableRead =1024;
	DWORD	dwRead;
	char	cCheckChar; 	
    bool bSuccess = true;//false;

	/* Check if there's available data in the pipe */
    /*bSuccess = PeekNamedPipe( self->readPipe,
							  &cCheckChar,
							  sizeof(char),
					          &dwRead,
						      &nAvailableRead,
							  NULL );*/
    if (!bSuccess || !nAvailableRead) {
        continue;
    }
        char data[BUFFER_SIZE];
        size_t bytes_count = self->read(data, nAvailableRead);//BUFFER_SIZE);
        if (bytes_count == 0)
            break;
        WaitForSingleObject(self->reading_mutex, INFINITE);
        if (bytes_count != 0)
        {
            data[bytes_count] = 0;
            self->output_buffer->write(data, bytes_count);
        }
        ReleaseMutex(self->reading_mutex);
    }
    return 0;
}

output_pipe_class::output_pipe_class(): pipe_class(PIPE_OUTPUT), output_buffer(&dummy_output_buffer)
{}
output_pipe_class::output_pipe_class(output_buffer_class *output_buffer_param): 
    pipe_class(PIPE_OUTPUT), output_buffer(output_buffer_param)
{}

bool output_pipe_class::bufferize()
{
    if (!pipe_class::bufferize())
        return false;
    buffer_thread = CreateThread(NULL, 0, reading_buffer, this, 0, NULL);
    if (!buffer_thread)
    {
        raise_error(*this, "CreateThread");
        return false;
    }
    return true;
}

pipe_t output_pipe_class::get_pipe()
{
    return output_pipe();
}

