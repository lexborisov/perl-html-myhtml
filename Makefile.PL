package MYINSTALLER;

use strict;
use utf8;

use FindBin;
use File::Find;
use ExtUtils::MakeMaker;

my $myhtml_source_dir = $FindBin::Bin ."/source";
our ($LIST_COMPILE, $LIST_OBJECTS);

sub find_all_obj_files {
	my ($dir_name) = @_;
	
	my @list;
	my @obj_list;
	
	my $search = sub {
		return unless $_ =~ /\.c$/;
		
		my $obj_name = $_; #$File::Find::name;
		$obj_name =~ s/\.c$/.o/;
		
		push @obj_list, "source/$obj_name";
		push @list, "-o source/$obj_name ". $File::Find::name;
	};
	
	find({ wanted => $search, follow => 1 }, "$dir_name/myhtml");
	
	return (\@list, \@obj_list);
}

($LIST_COMPILE, $LIST_OBJECTS) = find_all_obj_files($myhtml_source_dir);

my $fdfdf = WriteMakefile(
    AUTHOR         => 'Alexander Borisov <lex.borisov@gmail.com>',
    ABSTRACT       => 'Fast HTML Parser using Threads with no outside dependencies',
    VERSION_FROM   => 'MyHTML.pm',
    NAME           => 'HTML::MyHTML',
    LICENSE        => 'Apache 2.0',
    LINKTYPE       => 'dynamic',
    LIBS           => ["-lpthread"],
	OBJECT         => [@$LIST_OBJECTS, "MyHTML.o"],
	INC            => "-I$myhtml_source_dir",
	#MYEXTLIB       => "$myhtml_source_dir/myhtml/myhtml.o",
	clean          => {FILES => "$myhtml_source_dir/myhtml/*libmyhtml* $myhtml_source_dir/*.o $myhtml_source_dir/myhtml/*.o $myhtml_source_dir/myhtml/utils/*.o"}
);

package MY;
use Config;

sub postamble {
        my $self = shift;
		my @list;
		
		foreach my $name (@{$MYINSTALLER::LIST_COMPILE}) {
			push @list, "\t". '$(CC) -Wall -Werror -O2 -fPIC --std=c99 -pthread $(INC) -c '. $name;
		}
		
        return join "\n", @list;
}


