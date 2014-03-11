use File::Spec::Functions;
use Cwd 'abs_path';
use IPC::Run qw(start finish);
our @EXPORT = qw(sp_old execute_new);
my $sp_ext = '.exe';

sub path_to_system_specific {
    return catfile(split('/', $_[0]));
}

my $old_sp = path_to_system_specific(abs_path('../bin/old/sp'.$sp_ext));
my $new_sp = path_to_system_specific(abs_path('../bin/new/sp'.$sp_ext));

sub sp_old {
    my ($params, %rest) = @_;
    my $h = start [$new_sp, split(' ', $params)], \$in, \$out, \$err;
    $?==0 or die "Spawner execution finished with $?";
    finish $h or die;
    return $out;
}
1;