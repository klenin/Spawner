Модуль контролируемого исполнения программ "Spawner"
=======================================

Модуль разрабатывается как часть системы организации соревнований [Cats](https://github.com/klenin/cats-judge).
Модуль поддерживает различные режимы совместимости. Режим совместимости задается ключом `--legacy` или переменной окружения `SP_LEGACY`. Доступные значения можно посмотреть в списке поддерживаемых систем. Значение по-умолчанию `sp00`.
Справку по каждой из них можно получить запустив модуль с ключом `--help` или `-h`.

sp99
---------------------------------------
Изначальная версия sp взятая за основу этого проекта.
```
sp [опции] имя_приложения [параметры_приложения]
```
|Опции              |Переменные окружения |                                                                           |
|-------------------|---------------------|---------------------------------------------------------------------------|
|  -ml:[n]          |  SP_MEMORY_LIMIT    |  Максимальный объем виртуальной памяти, выделенный процессу (в Mb).       |
|  -tl:[n]          |  SP_TIME_LIMIT      |  Максимальное время выполнения процесса в пользовательском режиме (в сек).|
|  -d:[n]           |  SP_DEADLINE        |  Лимит физического времени, выделенного процессу (в сек).                 |
|  -wl:[n]          |  SP_WRITE_LIMIT     |  Максимальный объем данных, который может быть записан процессом (в Mb).  |
|  -u:[user@domain] |  SP_USER            |  Имя пользователя в формате: User[@Domain]                                |
|  -p:[password]    |  SP_PASSWORD        |  Пароль.                                                                  |
|  -runas:[0|1]     |  SP_RUNAS           |  Использовать сервис RunAs для запуска процесса.                          |
|  -s:[n]           |  SP_SECURITY_LEVEL  |  Уровень безопасности. Может принимать значения 0 или 1.                  |
|  -hr:[0|1]        |  SP_HIDE_REPORT     |  Не показывать отчет.                                                     |
|  -ho:[0|1]        |  SP_HIDE_OUTPUT     |  Не показывать выходной поток (STDOUT) приложения.                        |
|  -sr:[file]       |  SP_REPORT_FILE     |  Сохранить отчет в файл.                                                  |
|  -so:[file]       |  SP_OUTPUT_FILE     |  Сохранить выходной поток в файл.                                         |
|  -i:[file]        |  SP_INPUT_FILE      |  Получить входной поток из файла.                                         |
                                                                                                                      |

 Примечание: Параметры командной строки перекрывают значения переменных окружения.

                   
Формат файла отчета.
```
[пустая строка]
--------------- Spawner report ---------------
Application:            [Application]
Parameters:             [Parameters]
SecurityLevel:          [SecurityLevel]
CreateProcessMethod:    [CreateProcessMethod]
UserName:               [UserName]
UserTimeLimit:          [UserTimeLimit]
Deadline:               [Deadline]
MemoryLimit:            [MemoryLimit]
WriteLimit:             [WriteLimit]
----------------------------------------------
UserTime:               [UserTime]
PeakMemoryUsed:         [PeakMemoryUsed]
Written:                [Written]
TerminateReason:        [TerminateReason]
ExitStatus:             [ExitStatus]
----------------------------------------------
SpawnerError:           [SpawnerError]
```

* Application - Имя приложения
* Parameters - Параметры приложения
* SecurityLevel - Уровень защиты. Может быть 0 (по-умолчанию) или 1.
    Уровень 1 включает защиту от:
    * Выхода из системы, завершения ее работы, перезагрузки или выключения компьютера
    * Чтения, записи, стирания буфера обмена 
    * Изменения системных параметров через SystemParametersInfo 
    * Изменения параметров экрана через ChangeDisplaySettings 
    * Создания новых рабочих столов или переключения между ними
    * Использования USER-объектов (например, HWND), созданных внешними процессами 
    * Доступа к общей таблицы атомов (global atom table)

* CreateProcessMethod - Метод создания процесса. Может быть: "CreateProcess", "CreateProcessAsUser", "RunAs service"
        * "CreateProcess" - процесс запускается от текущего пользователя
        * "CreateProcessAsUser" - процесс запускается от заданного пользователя. 
        * "RunAs service" - процесс запускается при помощи сервиса RunAs.
        
        Запуск процесса от другого пользователя требует очень важных привилегий, а именно:
            SeTcbPrivilege (Работа в режиме операционной системы)
            SeAssignPrimaryTokenPrivilege (Замена маркера уровня процесса)
            SeIncreaseQuotaPrivilege (Увеличение квот)
        Запуск с помощью RunAs в некоторых конфигурациях может работать не корректно, 
        но он не требует специальных привилегий и может быть выполнен от обычного пользователя, 
        однако для этого должен быть включен сервис RunAs (SecondaryLogon).
 
* UserName - Имя пользователя под которым был запущен дочерний процесс в формате: User[@Domain]
* UserTimeLimit - максимальное время в сек. выполнения процесса в пользовательском режиме по истечении 
        которого процесс прерывается. По умолчанию: "Infinity".
* Deadline - Время в сек., которое выделено процессу. По умолчанию: "Infinity".
        Отличается от TimeLimit тем, что это физическое время.
        Если процесс непрерывно осуществляет ввод/вывод, находиться в состоянии ожидания или система перегружена, 
        то процесс может выполнятся неограниченно долго несмотря на TimeLimit. 
        Для предотвращения данной ситуации нужно установить DeadLine.
* MemoryLimit - Максимальный объем выделяемой памяти процессу в Mb. По умолчанию: "Infinity"
* WriteLimit - Максимальный объем информации, который может быть записан процессом в Mb. По умолчанию: "Infinity"

* UserTime - Фактическое время выполнения процесса в сек. с точностью до 10e-3.
* PeakMemoryUsed - Максимальное использование виртуальной памяти процессом в Mb.
* Written - Объем информации, который был записан процессом в Mb.
* TerminateReason - Причина завершения процесса. Может быть: 
        * "ExitProcess" - процесс завершился нормально
        * "MemoryLimitExceeded" - превышен лимит памяти
        * "TimeLimitExceeded" - превышен лимит времени выполнения (либо TimeLimit, либо Deadline)
        * "WriteLimitExceeded" - превышен лимит записи
        * "AbormalExitProcess" - процесс завершился с исключением (список исключений см. ниже)
        * Если процесс не был завершен, то данному полю соответствует значение "<none>"
                
* ExitStatus - Статус завершения процесса. Может принимать значение кода возврата процесса, либо одной из 
        следующих констант:
        * "AccessViolation"
        * "ArrayBoundsExceeded"
        * "Breakpoint"
        * "Control_C_Exit"
        * "DatatypeMisalignment"
        * "FloatDenormalOperand"
        * "FloatInexactResult"
        * "FloatInvalidOperation"
        * "FloatMultipleFaults"
        * "FloatMultipleTraps"
        * "FloatOverflow"
        * "FloatStackCheck"
        * "FloatUnderflow"
        * "GuardPageViolation"
        * "IllegalInstruction"
        * "InPageError"
        * "InvalidDisposition"
        * "IntegerDivideByZero"
        * "IntegerOverflow"
        * "NoncontinuableException"
        * "PrivilegedInstruction"
        * "RegNatConsumption"
        * "SingleStep"
        * "StackOverflow"
* SpawnerError - Текст ошибки при работе spawner'а. 
        Если ошибка не произошла, то полю соответствует значение "<none>"


sp00
---------------------------------------------
|Опции                 | Переменные окружения |                         |
|----------------------|----------------------|-------------------------|
|tl                    | SP_TIME_LIMIT        |                         |
|ml                    | SP_MEMORY_LIMIT      |                         |
|s                     | SP_SECURITY_LEVEL    |                         |
|d                     | SP_DEADLINE          |                         |
|wl                    | SP_WRITE_LIMIT       |                         |
|-lr                   | SP_LOAD_RATIO        |                         |
|-y                    | SP_IDLE_TIME_LIMIT   |                         |
|-u                    | SP_USER              |                         |
|-p                    | SP_PASSWORD          |                         |
|-sr                   | SP_REPORT_FILE       |                         |
|-so, --out            | SP_OUTPUT_FILE       |                         |
|-e, -se, --err        | SP_ERROR_FILE        |                         |
|-i, --in              | SP_INPUT_FILE        |                         |
|--delegated           | SP_RUNAS             |                         |
|--debug               | SP_DEBUG             |                         |
|--cmd, --systempath   | SP_SYSTEM_PATH       |                         |
|-wd                   | SP_DIRECTORY         |                         |
|-ho                   | SP_HIDE_OUTPUT       |                         |
|-hr                   | SP_HIDE_REPORT       |                         |
|-sw                   | SP_SHOW_WINDOW       |                         |
|--session             |   --                 |                         |
|--separator           | SP_SEPARATOR         |                         |
|--program             | SP_PROGRAM           |                         |
pcms2
---------------------------------------------
|Опции                                                  |                                                                                                                                                        |
|-------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------|
|  -h                                                   | show this help                                                                                                                                         |
|  -t <time-limit>                                      | time limit, terminate after <time-limit> seconds, you can add "ms" (without quotes) after the number to specify time limit in milliseconds             |
|  -m <mem-limit>                                       | memory limit, terminate if working set of the process exceeds <mem-limit> bytes, you can add K or M to specify memory limit in kilo- or megabytes      |
|  -r <req-load>                                        | required load of the processor for this process not to be considered idle. You can add % sign to specify required load in percent, default is 0.05 = 5%|
|  -y <idle-limit>                                      | ildeness limit, terminate process if it did not load processor for at least <req-load> for <idleness-limit>                                            |
|  -d <directory>                                       | make <directory> home directory for process                                                                                                            |
|  -l <login-name>                                      | create process under <login-name>                                                                                                                      |
|  -p <password>                                        | logins user using <password>                                                                                                                           |
|  -i <file>                                            | redirects standart input stream to the <file>                                                                                                          |
|  -o <file>                                            | redirects standart output stream to the <file>                                                                                                         |
|  -e <file>                                            | redirects standart error stream to the <file>                                                                                                          |
|  -x                                                   | return exit code of the application                                                                                                                    |
|  -q                                                   | do not display any information on the screen                                                                                                           |
|  -w                                                   | display program window on the screen                                                                                                                   |
|  -1                                                   | use single CPU/CPU core                                                                                                                                |
|  -s <file>                                            | store statistics in then <file>                                                                                                                        |
|  -D var=value                                         | sets value of the environment variable, current environment is completly ignored in this case                                                          |
|Exteneded options:                                     |                                                                                                                                                        |
|  -Xacp,                                               |                                                                                                                                                        |
|  --allow-create-processes                             | allow the created process to create new processes                                                                                                      |
|  -Xtfce, --terminate-on-first-chance-exceptions       | do not ignore exceptions if they are marked as first-chance, required for some old compilers as Borland Delphi                                         |
|  -Xlegacy, -z                                         | try to be compatible with old invoke.dll                                                                                                               |

Examples:
```
  sp -t 10s -m 32M -i 10s a.exe
  sp -d "C:\My Directory" a.exe
  sp -l invoker -p password a.exe
  sp -i input.txt -o output.txt -e error.txt a.exe
```