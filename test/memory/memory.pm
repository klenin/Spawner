use Test::More;
use spawner '..';
use Cwd;
use strict;

our @EXPORT = qw(memory_run_tests);

sub memory_limit_test {
    my $rep = sp_old("-sr=rep.txt bin/memory_alloc.exe 10000000 1");
    ok(parse_report($rep, 1)!=undef, "report is ok");

    ok(1==1, "ok?");
    ok(1==1, "ok?");
    ok(1==1, "ok?");
    ok(1==1, "ok?");
    done_testing();
}

sub memory_run_tests {
    ok(1==1, 'memory');
    subtest 'Testing memory limit' => sub { memory_limit_test() };
    1;
}