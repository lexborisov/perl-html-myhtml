use strict;
use utf8;

use FindBin;
use ExtUtils::MakeMaker;
use Config;

my $myhtml_source_dir = $FindBin::Bin ."/source";
my $myhtml_lib_dir = "$myhtml_source_dir/lib";

print "Build MyHTML library: ";
my $res = `make -j4 -C $myhtml_source_dir`;

unless ($?) {
	print "done\n";
}
else {
	print "fault\n";
}

WriteMakefile(
    AUTHOR         => 'Alexander Borisov <lex.borisov@gmail.com>',
    ABSTRACT_FROM  => 'MyHTML.pm',
    VERSION_FROM   => 'MyHTML.pm',
    NAME           => 'HTML::MyHTML',
    LICENSE        => 'Apache 2.0',
    LINKTYPE       => 'dynamic',
	MYEXTLIB       => "$myhtml_lib_dir/static_libmyhtml.a",
    LIBS           => ["-lpthread"],
    INC            => "-I$myhtml_source_dir",
	clean          => {FILES => "$myhtml_source_dir/myhtml/*libmyhtml* $myhtml_source_dir/myhtml/*.o $myhtml_source_dir/myhtml/utils/*.o $myhtml_lib_dir/*"}
);


