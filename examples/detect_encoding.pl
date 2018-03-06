#!/usr/bin/perl -w

use utf8;
use strict;

use HTML::MyHTML;
use LWP::UserAgent;

my $ua = LWP::UserAgent->new;
my $req = HTTP::Request->new(GET => "https://www.yandex.ru/");
my $res = $ua->request($req);

my $body = $res->content;

# init
my $myhtml = HTML::MyHTML->new(MyHTML_OPTIONS_DEFAULT, 1);
my $tree = $myhtml->new_tree();

# detect encoding
my $encoding = prescan_stream_to_determine_encoding($body);

# parse
$myhtml->parse($tree, MyENCODING_UTF_8, $body);

# print result
print "Print HTML Tree:\n";
$tree->document->print_children(*STDOUT);


$tree->destroy();

