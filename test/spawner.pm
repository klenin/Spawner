use File::Spec::Functions;
use Cwd 'abs_path';
use IPC::Run qw(start finish);
use Data::Dumper;

my @required_fields = (
    'Application', 
    'Parameters', 
    'SecurityLevel', 
    'CreateProcessMethod',
    'UserName',
    'UserTimeLimit',
    'DeadLine',
    'MemoryLimit',
    'WriteLimit',
    'UserTime',
    'PeakMemoryUsed',
    'Written',
    'TerminateReason',
    'ExitStatus',
    'SpawnerError'
);

our @EXPORT = qw(sp_old sp_new parse_report sp_compete);
my $sp_ext = '.exe';

sub path_to_system_specific {
    return catfile(split('/', $_[0]));
}

my $old_sp = path_to_system_specific(abs_path('../bin/old/sp'.$sp_ext));
my $new_sp = path_to_system_specific(abs_path('../bin/new/sp'.$sp_ext));

sub sp_from_disk {
    open REPORT, "<$_[0]";
    open OUT, "<$_[1]";
    my $report = do { local $/; <REPORT> };    
    my $stdout = do { local $/; <OUT> };    
    return ($report, $stdout);
}
sub sp_old {
    my ($params, %rest) = @_;
    my $status = system("$old_sp -so:stdout.txt -sr:report.txt -hr:1 $params") >> 8;
    $status == 0 or die "Old spawner execution finished with $status";
    return sp_from_disk("report.txt", "stdout.txt");
}
sub sp_new {
    my ($params, %rest) = @_;
    my $h = start [$new_sp, split(' ', $params)], \$in, \$out, \$err;
    $?==0 or die "New spawner execution finished with $?";
    finish $h or die;
    return $out;
}
sub parse_report {
    my ($report, $check) = @_;
    my %report_object = {};

    # Пример файла отчета:
    #
    #--------------- Spawner report ---------------
    #Application:           test.exe
    #Parameters:            <none>
    #SecurityLevel:         0
    #CreateProcessMethod:   CreateProcessAsUser
    #UserName:              acm3
    #UserTimeLimit:         0.001000 (sec)
    #DeadLine:              Infinity
    #MemoryLimit:           20.000000 (Mb)
    #WriteLimit:            Infinity
    #----------------------------------------------
    #UserTime:              0.010014 (sec)
    #PeakMemoryUsed:        20.140625 (Mb)
    #Written:               0.000000 (Mb)
    #TerminateReason:       TimeLimitExceeded
    #ExitStatus:            0
    #----------------------------------------------
    #SpawnerError:          <none>
    my $checking = 0;
    foreach (split('\n', $report)) {
        if ($_ =~ /^(.+):\s+(.+)$/) {
            my $p = $1;
            my $v = $2;
            if ($v =~ /^(\d+\.?\d+)\s*\((.+)\)/) {
                $v = $1;
                #check $2
            }
            if ($check) {
                if ($p != $required_fields[$checking]) {
                    print "Wrong position for $p\n";
                    return undef;
                }
                $checking++;
            }
            $report_object{$p} = $v;
        }
    }
    #print Dumper(\%report_object);
    return %report_object;
}

sub sp_compete {
    my ($params, $check) = @_;
    my ($report, $stdout) = sp_old($params);

    my %old_report = parse_report($report, $check);
    my %new_report = parse_report(sp_new($params), $check);
    return (\%old_report, \%new_report);
}


1;