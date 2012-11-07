#include <ParseArguments.h>
#include <iostream>
#include <string>

#include <spawner.h>
#include <ddk/ntifs.h>
#include <ddk/ntstatus.h>
typedef struct
{
    /** The length, in bytes, of the string. */
    SIZE_T Length;
    /** The buffer containing the contents of the string. */
    PWSTR Buffer;
} _PH_STRINGREF;
typedef _PH_STRINGREF PH_STRINGREF;
typedef _PH_STRINGREF *PPH_STRINGREF;

void PhInitializeStringRef(
    PPH_STRINGREF String,
    PWSTR Buffer
    )
{
    String->Length = wcslen(Buffer) * sizeof(WCHAR);
    String->Buffer = Buffer;
}
HANDLE PhKphHandle = NULL;
#define PhInitFormatS(f, v) do { (f)->Type = StringFormatType; PhInitializeStringRef(&(f)->u.String, (v)); } while (0)
// typedef struct
// {
//     USHORT Length;
//     USHORT MaximumLength;
//     field_bcount_part_opt(MaximumLength, Length) PCHAR Buffer;
// } STRING;
//typedef _STRING STRING;
typedef STRING *PSTRING;
typedef STRING ANSI_STRING;
typedef STRING *PANSI_STRING;
typedef STRING OEM_STRING;
typedef STRING *POEM_STRING;
typedef enum
{
    CharFormatType,
    StringFormatType,
    StringZFormatType,
    AnsiStringFormatType,
    AnsiStringZFormatType,
    Int32FormatType,
    Int64FormatType,
    IntPtrFormatType,
    UInt32FormatType,
    UInt64FormatType,
    UIntPtrFormatType,
    DoubleFormatType,
    SizeFormatType,
    FormatTypeMask = 0x3f,

    /** If not specified, for floating-point 6 is assumed **/
    FormatUsePrecision = 0x40,
    /** If not specified, ' ' is assumed */
    FormatUsePad = 0x80,
    /** If not specified, 10 is assumed */
    FormatUseRadix = 0x100,
    /** If not specified, the default value is assumed */
    FormatUseParameter = 0x200,

    // Floating-point flags
    /** Use standard form instead of normal form */
    FormatStandardForm = 0x1000,
    /** Use hexadecimal form instead of normal form */
    FormatHexadecimalForm = 0x2000,
    /** Reserved */
    FormatForceDecimalPoint = 0x4000,
    /** Trailing zeros and possibly the decimal point are trimmed */
    FormatCropZeros = 0x8000,

    // Floating-point and integer flags
    /** Group digits (with floating-point, only works when in normal form) */
    FormatGroupDigits = 0x10000,
    /** Always insert a prefix, '+' for positive and '-' for negative */
    FormatPrefixSign = 0x20000,
    /** Pad left with zeros, taking into consideration the sign. Width must be specified.
     * Format*Align cannot be used in conjunction with this flag. If FormatGroupDigits is specified,
     * this flag is ignored. */
    FormatPadZeros = 0x40000,

    // General flags
    /** Applies left alignment. Width must be specified. */
    FormatLeftAlign = 0x80000000,
    /** Applies right alignment. Width must be specified. */
    FormatRightAlign = 0x40000000,
    /** Make characters uppercase (only available for some types) */
    FormatUpperCase = 0x20000000
} PH_FORMAT_TYPE;

//typedef struct
//{
//    /** The length, in bytes, of the string. */
//    SIZE_T Length;
//    /** The buffer containing the contents of the string. */
//    PWSTR Buffer;
//} PH_STRINGREF, *PPH_STRINGREF;
typedef struct
{
    union
    {
        struct
        {
            /** The length, in bytes, of the string. */
            USHORT Length;
            /** Unused and of an undefined value. */
            USHORT Reserved;
            /** The buffer containing the contents of the string. */
            PSTR Buffer;
        };
        ANSI_STRING as;
    };
}_PH_ANSI_STRINGREF;
typedef _PH_ANSI_STRINGREF PH_ANSI_STRINGREF;
typedef _PH_ANSI_STRINGREF *PPH_ANSI_STRINGREF;

//typedef struct _PH_FORMAT
//{
//    /** Specifies the type of the element and optional flags. */
//    PH_FORMAT_TYPE Type;
//    /** The precision of the element. The meaning of this field depends on
//     * the element type. For \a Double and \a Size, this field specifies
//     * the number of decimal points to include. */
//    USHORT Precision;
//    /** The width of the element. This field specifies the minimum
//     * number of characters to output. The remaining space is
//     * padded with either spaces, zeros, or a custom character. */
//    USHORT Width;
//    /** The pad character. */
//    WCHAR Pad;
//    /** The meaning of this field depends on the element type. For integer
//     * types, this field specifies the base to convert the number into.
//     * For \a Size, this field specifies the maximum size unit. */
//    UCHAR Radix;
//    /** The meaning of this field depends on the element type. For \a Size,
//     * this field specifies the minimum size unit. */
//    UCHAR Parameter;
//    union
//    {
//        WCHAR Char;
//        PH_STRINGREF String;
//        PWSTR StringZ;
//        PH_ANSI_STRINGREF AnsiString;
//        PSTR AnsiStringZ;
//        LONG Int32;
//        LONG64 Int64;
//        LONG_PTR IntPtr;
//        ULONG UInt32;
//        ULONG64 UInt64;
//        ULONG_PTR UIntPtr;
//        DOUBLE Double;
//
//        ULONG64 Size;
//    } u;
//} PH_FORMAT, *PPH_FORMAT;
typedef enum// _KPH_SECURITY_LEVEL
{
    KphSecurityNone = 0, // all clients are allowed
    KphSecurityPrivilegeCheck = 1, // require SeDebugPrivilege
    KphMaxSecurityLevel
} KPH_SECURITY_LEVEL;

typedef KPH_SECURITY_LEVEL *PKPH_SECURITY_LEVEL;
typedef struct
{
    KPH_SECURITY_LEVEL SecurityLevel;
    BOOLEAN CreateDynamicConfiguration;
} _KPH_PARAMETERS;
typedef _KPH_PARAMETERS KPH_PARAMETERS;
typedef _KPH_PARAMETERS *PKPH_PARAMETERS;
typedef struct
{
    BOOLEAN Inherit;
    BOOLEAN ProtectFromClose;
} _OBJECT_HANDLE_FLAG_INFORMATION;
typedef _OBJECT_HANDLE_FLAG_INFORMATION OBJECT_HANDLE_FLAG_INFORMATION;
typedef _OBJECT_HANDLE_FLAG_INFORMATION *POBJECT_HANDLE_FLAG_INFORMATION;
void RtlInitUnicodeString(
    PUNICODE_STRING DestinationString,
    PWSTR SourceString
    )
{
    if (SourceString)
        DestinationString->MaximumLength = (DestinationString->Length = (USHORT)(wcslen(SourceString) * sizeof(WCHAR))) + sizeof(WCHAR);
    else
        DestinationString->MaximumLength = DestinationString->Length = 0;

    DestinationString->Buffer = SourceString;
}
NTSTATUS KphConnect(
    PWSTR DeviceName
    )
{
    NTSTATUS status;
    HANDLE kphHandle;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK isb;
    OBJECT_HANDLE_FLAG_INFORMATION handleFlagInfo;

    if (PhKphHandle)
        return STATUS_ADDRESS_ALREADY_EXISTS;

    RtlInitUnicodeString(&objectName, DeviceName);

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
        &kphHandle,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        &objectAttributes,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_NON_DIRECTORY_FILE
        );

    if (NT_SUCCESS(status))
    {
        // Protect the handle from being closed.

        handleFlagInfo.Inherit = FALSE;
        handleFlagInfo.ProtectFromClose = TRUE;

        NtSetInformationObject(
            kphHandle,
            ObjectHandleInformation,
            &handleFlagInfo,
            sizeof(OBJECT_HANDLE_FLAG_INFORMATION)
            );

        PhKphHandle = kphHandle;
    }

    return status;
}
NTSTATUS KphConnect2Ex(
    PWSTR DeviceName,
    /*__in*/ PWSTR FileName,
    /*__in_opt*/ PKPH_PARAMETERS Parameters
    )
{
    NTSTATUS status;
    WCHAR fullDeviceName[256] = L"\\Device\\Spawner";
//    PH_FORMAT format[2];
    SC_HANDLE scmHandle;
    SC_HANDLE serviceHandle;
    BOOLEAN started = FALSE;
    BOOLEAN created = FALSE;

    //  if (!DeviceName)
    //      DeviceName = L"Spawner";

    //    PhInitFormatS(&format[0], L"\\Device\\");
    //    PhInitFormatS(&format[1], DeviceName);


    // Try to open the device.
    status = KphConnect(fullDeviceName);

    if (NT_SUCCESS(status) || status == STATUS_ADDRESS_ALREADY_EXISTS)
        return status;

    return status;
}

NTSTATUS KphConnect2(
    /*__in_opt*/ PWSTR DeviceName,
    /*__in*/ PWSTR FileName
    )
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    return KphConnect2Ex(DeviceName, FileName, NULL);
}


NTSTATUS KphpDeviceIoControl(
    /*__in*/ ULONG KphControlCode,
    /*__in*/ PVOID InBuffer,
    /*__in*/ ULONG InBufferLength
    )
{
    IO_STATUS_BLOCK isb;

    return NtDeviceIoControlFile(
        PhKphHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        KphControlCode,
        InBuffer,
        InBufferLength,
        NULL,
        0
        );
}
NTSTATUS KphOpenProcessJob(
    /*__in*/ HANDLE ProcessHandle,
    /*__in*/ ACCESS_MASK DesiredAccess,
    /*__out*/ PHANDLE JobHandle
    )
{
    struct
    {
        HANDLE ProcessHandle;
        ACCESS_MASK DesiredAccess;
        PHANDLE JobHandle;
    } input = { ProcessHandle, DesiredAccess, JobHandle };

    return KphpDeviceIoControl(
        CTL_CODE(0x9999, 0x800 + 52, METHOD_NEITHER, FILE_ANY_ACCESS),
        &input,
        sizeof(input)
        );
}

// Formatting report
std::string format_report(report_class rep)
{
    std::ostringstream osstream;
    osstream << std::endl << "--------------- Spawner report ---------------" << std::endl;
    osstream << "Application:               " << rep.application_name << std::endl;
    osstream << "Working directory:         " << rep.options.working_directory << std::endl;
    osstream << "Parameters:                " << rep.options.get_arguments() << std::endl;
    osstream << "Silent:                    " << rep.options.silent_errors << std::endl;
    osstream << "Debug:                     " << rep.options.debug << std::endl;
    osstream << "HideUI:                    " << rep.options.hide_gui << std::endl;
    osstream << "SecurityLevel:             " << (rep.restrictions.get_restriction(restriction_security_limit) == restriction_limited) << std::endl;
    osstream << "CreateProcessMethod:       " << (rep.options.login==""?"Default":"WithLogon") << std::endl;
    osstream << "UserName:                  " << rep.options.login << std::endl;
    osstream << "TimeLimit:                 " << convert(value_t(unit_time_second, degree_milli), value_t(unit_time_second), rep.restrictions.get_restriction(restriction_processor_time_limit), " (u)", restriction_no_limit) << std::endl;
    osstream << "DeadLine:                  " << convert(value_t(unit_time_second, degree_milli), value_t(unit_time_second), rep.restrictions.get_restriction(restriction_user_time_limit), " (u)", restriction_no_limit) << std::endl;
    osstream << "MemoryLimit:               " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), rep.restrictions.get_restriction(restriction_memory_limit), " (du)", restriction_no_limit) << std::endl;
    osstream << "WriteLimit:                " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), rep.restrictions.get_restriction(restriction_write_limit), " (du)", restriction_no_limit) << std::endl;
    osstream << "LoadRatioLimit:            " << convert(value_t(unit_no_unit, degree_m4), value_t(unit_no_unit), rep.restrictions.get_restriction(restriction_load_ratio), " (%)", restriction_no_limit) << std::endl;
    osstream << "----------------------------------------------" << std::endl;
    osstream << "ProcessorTime:             " << convert(value_t(unit_time_second, degree_micro), value_t(unit_time_second), rep.processor_time/10.0, " (u)") << std::endl;
    osstream << "KernelTime:                " << convert(value_t(unit_time_second, degree_micro), value_t(unit_time_second), rep.kernel_time/10.0, " (u)") << std::endl;
    osstream << "UserTime:                  " << convert(value_t(unit_time_second, degree_milli), value_t(unit_time_second), rep.user_time, " (u)") << std::endl;
    osstream << "PeakMemoryUsed:            " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), rep.peak_memory_used, " (du)") << std::endl;
    osstream << "Written:                   " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), rep.write_transfer_count, " (du)") << std::endl;
    osstream << "LoadRatio:                 " << convert(value_t(unit_no_unit, degree_centi), value_t(unit_no_unit), rep.load_ratio, " (%)", restriction_no_limit) << std::endl;
    osstream << "TerminateReason:           " << get_terminate_reason(rep.terminate_reason) << std::endl;
    osstream << "ExitStatus:                " << get_status_text(rep.process_status) << std::endl;
    osstream << "ExitCode:                  " << rep.exit_code << std::endl;
    osstream << "Exception:                 " << get_exception_name(rep.exception) << std::endl;
    osstream << "ExceptionInterpretation:   " << get_exception_text(rep.exception) << std::endl;
    osstream << "----------------------------------------------" << std::endl;
    osstream << "SpawnerError:              " << std::endl;
    return osstream.str();
}

int main(int argc, char *argv[])
{
#ifdef _MSC_VER
    BOOL x;
    IsProcessInJob(GetCurrentProcess(), NULL, &x);
#endif
    NTSTATUS status = KphConnect2(L"sp", L"spawner.sys");
    std::cout << status;
	CArguments arguments(argc, argv);
    if (!arguments.valid())
    {
        arguments.ShowUsage();
        return 0;
    }

    restrictions_class restrictions;
    options_class options;

    for (int i = arguments.get_arguments_index(); i < argc; i++)
        options.add_argument(argv[i]);

    //make this automatic

    if (arguments.ArgumentExists(SP_MEMORY_LIMIT))
    {
        restrictions.set_restriction(restriction_memory_limit, convert(value_t(unit_memory_byte),
            arguments.GetArgument(SP_MEMORY_LIMIT), restriction_no_limit));
    }

    if (arguments.ArgumentExists(SP_WRITE_LIMIT))
    {
        restrictions.set_restriction(restriction_write_limit, convert(value_t(unit_memory_byte),
            arguments.GetArgument(SP_WRITE_LIMIT), restriction_no_limit));
    }

    if (arguments.ArgumentExists(SP_TIME_LIMIT))
    {
        restrictions.set_restriction(restriction_processor_time_limit, convert(value_t(unit_time_second, degree_milli),
            arguments.GetArgument(SP_TIME_LIMIT), restriction_no_limit));
    }

    if (arguments.ArgumentExists(SP_DEADLINE))
    {
        restrictions.set_restriction(restriction_user_time_limit, convert(value_t(unit_time_second, degree_milli),
            arguments.GetArgument(SP_DEADLINE), restriction_no_limit));
    }

    if (arguments.ArgumentExists(SP_LOAD_RATIO))
    {
        restrictions.set_restriction(restriction_load_ratio, convert(value_t(unit_no_unit, degree_m4),
            arguments.GetArgument(SP_LOAD_RATIO), restriction_no_limit));//dirty hack
    }

    if (arguments.ArgumentExists(SP_WORKING_DIRECTORY))
        options.working_directory = arguments.GetArgument(SP_WORKING_DIRECTORY);

    if (arguments.ArgumentExists(SP_SILENT))
        options.silent_errors = true;
    if (arguments.ArgumentExists(SP_LOGIN))
        options.login = arguments.GetArgument(SP_LOGIN);
    if (arguments.ArgumentExists(SP_PASSWORD))
        options.password = arguments.GetArgument(SP_PASSWORD);
    options.hide_gui = true;

    process_wrapper wrapper(arguments.get_program(), options, restrictions);
    wrapper.run_process();
    report_class rep = wrapper.get_report();

    std::cout << format_report(rep);
	return 0;
}
