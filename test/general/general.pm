use Test::More;
use spawner '..';
use Cwd;
use Data::Dumper;
use strict;

our @EXPORT = qw(general_run_tests);

sub threshold {
    return @_[0] > 0.90 && @_[0] < 1.1;
}

sub report_test {
    my ($or, $nr) = &sp_compete("general_simple.exe", 1);
    my %old_report = %{$or};
    my %new_report = %{$nr};


    ok(%old_report!=undef, "Old spawner report");
    ok(%new_report!=undef, "New spawner report");

    ok(threshold($old_report{PeakMemoryUsed}/$new_report{PeakMemoryUsed}), "Memory usage match"); 
    ok($old_report{Written} == $new_report{Written}, "Written file"); 
    #ok(threshold($old_report{UserTime}/$new_report{UserTime}), "User time match"); disabled

    done_testing();
}

sub general_run_tests {
    subtest 'Testing report' => sub { report_test() };
    1;
}