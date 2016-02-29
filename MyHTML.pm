#
# Copyright 2015-2016 Alexander Borisov
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 
# Author: lex.borisov@gmail.com (Alexander Borisov)
#
package HTML::MyHTML;

use utf8;
use strict;
use vars qw($AUTOLOAD $VERSION $ABSTRACT @ISA @EXPORT);

BEGIN {
	$VERSION = 0.24;
	$ABSTRACT = "Fast HTML Parser using Threads with no outside dependencies";
	
	@ISA = qw(Exporter DynaLoader);
	@EXPORT = qw(
		MyHTML_ENCODING_DEFAULT MyHTML_ENCODING_UTF_8 MyHTML_ENCODING_UTF_16LE MyHTML_ENCODING_UTF_16BE
		MyHTML_ENCODING_X_USER_DEFINED MyHTML_ENCODING_BIG5 MyHTML_ENCODING_EUC_KR MyHTML_ENCODING_GB18030
		MyHTML_ENCODING_IBM866 MyHTML_ENCODING_ISO_8859_10 MyHTML_ENCODING_ISO_8859_13 MyHTML_ENCODING_ISO_8859_14
		MyHTML_ENCODING_ISO_8859_15 MyHTML_ENCODING_ISO_8859_16 MyHTML_ENCODING_ISO_8859_2 MyHTML_ENCODING_ISO_8859_3
		MyHTML_ENCODING_ISO_8859_4 MyHTML_ENCODING_ISO_8859_5 MyHTML_ENCODING_ISO_8859_6 MyHTML_ENCODING_ISO_8859_7
		MyHTML_ENCODING_ISO_8859_8 MyHTML_ENCODING_KOI8_R MyHTML_ENCODING_KOI8_U MyHTML_ENCODING_MACINTOSH
		MyHTML_ENCODING_WINDOWS_1250 MyHTML_ENCODING_WINDOWS_1251 MyHTML_ENCODING_WINDOWS_1252 MyHTML_ENCODING_WINDOWS_1253
		MyHTML_ENCODING_WINDOWS_1254 MyHTML_ENCODING_WINDOWS_1255 MyHTML_ENCODING_WINDOWS_1256 MyHTML_ENCODING_WINDOWS_1257
		MyHTML_ENCODING_WINDOWS_1258 MyHTML_ENCODING_WINDOWS_874 MyHTML_ENCODING_X_MAC_CYRILLIC MyHTML_ENCODING_LAST_ENTRY
		
		MyHTML_TAG__UNDEF MyHTML_TAG__TEXT MyHTML_TAG__COMMENT MyHTML_TAG__DOCTYPE MyHTML_TAG_A MyHTML_TAG_ABBR
		MyHTML_TAG_ACRONYM MyHTML_TAG_ADDRESS MyHTML_TAG_ANNOTATION_XML MyHTML_TAG_APPLET MyHTML_TAG_AREA MyHTML_TAG_ARTICLE
		MyHTML_TAG_ASIDE MyHTML_TAG_AUDIO MyHTML_TAG_B MyHTML_TAG_BASE MyHTML_TAG_BASEFONT MyHTML_TAG_BDI MyHTML_TAG_BDO
		MyHTML_TAG_BGSOUND MyHTML_TAG_BIG MyHTML_TAG_BLINK MyHTML_TAG_BLOCKQUOTE MyHTML_TAG_BODY MyHTML_TAG_BR
		MyHTML_TAG_BUTTON MyHTML_TAG_CANVAS MyHTML_TAG_CAPTION MyHTML_TAG_CENTER MyHTML_TAG_CITE MyHTML_TAG_CODE
		MyHTML_TAG_COL MyHTML_TAG_COLGROUP MyHTML_TAG_COMMAND MyHTML_TAG_COMMENT MyHTML_TAG_DATALIST MyHTML_TAG_DD MyHTML_TAG_DEL
		MyHTML_TAG_DETAILS MyHTML_TAG_DFN MyHTML_TAG_DIALOG MyHTML_TAG_DIR MyHTML_TAG_DIV MyHTML_TAG_DL MyHTML_TAG_DT MyHTML_TAG_EM
		MyHTML_TAG_EMBED MyHTML_TAG_FIELDSET MyHTML_TAG_FIGCAPTION MyHTML_TAG_FIGURE MyHTML_TAG_FONT MyHTML_TAG_FOOTER
		MyHTML_TAG_FORM MyHTML_TAG_FRAME MyHTML_TAG_FRAMESET MyHTML_TAG_H1 MyHTML_TAG_H2 MyHTML_TAG_H3 MyHTML_TAG_H4
		MyHTML_TAG_H5 MyHTML_TAG_H6 MyHTML_TAG_HEAD MyHTML_TAG_HEADER MyHTML_TAG_HGROUP MyHTML_TAG_HR MyHTML_TAG_HTML
		MyHTML_TAG_I MyHTML_TAG_IFRAME MyHTML_TAG_IMAGE MyHTML_TAG_IMG MyHTML_TAG_INPUT MyHTML_TAG_INS MyHTML_TAG_ISINDEX
		MyHTML_TAG_KBD MyHTML_TAG_KEYGEN MyHTML_TAG_LABEL MyHTML_TAG_LEGEND MyHTML_TAG_LI MyHTML_TAG_LINK MyHTML_TAG_LISTING
		MyHTML_TAG_MAIN MyHTML_TAG_MAP MyHTML_TAG_MARK MyHTML_TAG_MARQUEE MyHTML_TAG_MENU MyHTML_TAG_MENUITEM MyHTML_TAG_META
		MyHTML_TAG_METER MyHTML_TAG_MTEXT MyHTML_TAG_NAV MyHTML_TAG_NOBR MyHTML_TAG_NOEMBED MyHTML_TAG_NOFRAMES MyHTML_TAG_NOSCRIPT
		MyHTML_TAG_OBJECT MyHTML_TAG_OL MyHTML_TAG_OPTGROUP MyHTML_TAG_OPTION MyHTML_TAG_OUTPUT MyHTML_TAG_P MyHTML_TAG_PARAM
		MyHTML_TAG_PLAINTEXT MyHTML_TAG_PRE MyHTML_TAG_PROGRESS MyHTML_TAG_Q MyHTML_TAG_RB MyHTML_TAG_RP MyHTML_TAG_RT MyHTML_TAG_RTC
		MyHTML_TAG_RUBY MyHTML_TAG_S MyHTML_TAG_SAMP MyHTML_TAG_SCRIPT MyHTML_TAG_SECTION MyHTML_TAG_SELECT MyHTML_TAG_SMALL
		MyHTML_TAG_SOURCE MyHTML_TAG_SPAN MyHTML_TAG_STRIKE MyHTML_TAG_STRONG MyHTML_TAG_STYLE MyHTML_TAG_SUB MyHTML_TAG_SUMMARY
		MyHTML_TAG_SUP MyHTML_TAG_SVG MyHTML_TAG_TABLE MyHTML_TAG_TBODY MyHTML_TAG_TD MyHTML_TAG_TEMPLATE MyHTML_TAG_TEXTAREA
		MyHTML_TAG_TFOOT MyHTML_TAG_TH MyHTML_TAG_THEAD MyHTML_TAG_TIME MyHTML_TAG_TITLE MyHTML_TAG_TR MyHTML_TAG_TRACK
		MyHTML_TAG_TT MyHTML_TAG_U MyHTML_TAG_UL MyHTML_TAG_VAR MyHTML_TAG_VIDEO MyHTML_TAG_WBR MyHTML_TAG_XMP MyHTML_TAG_ALTGLYPH
		MyHTML_TAG_ALTGLYPHDEF MyHTML_TAG_ALTGLYPHITEM MyHTML_TAG_ANIMATE MyHTML_TAG_ANIMATECOLOR MyHTML_TAG_ANIMATEMOTION
		MyHTML_TAG_ANIMATETRANSFORM MyHTML_TAG_CIRCLE MyHTML_TAG_CLIPPATH MyHTML_TAG_COLOR_PROFILE MyHTML_TAG_CURSOR MyHTML_TAG_DEFS
		MyHTML_TAG_DESC MyHTML_TAG_ELLIPSE MyHTML_TAG_FEBLEND MyHTML_TAG_FECOLORMATRIX MyHTML_TAG_FECOMPONENTTRANSFER MyHTML_TAG_FECOMPOSITE
		MyHTML_TAG_FECONVOLVEMATRIX MyHTML_TAG_FEDIFFUSELIGHTING MyHTML_TAG_FEDISPLACEMENTMAP MyHTML_TAG_FEDISTANTLIGHT
		MyHTML_TAG_FEDROPSHADOW MyHTML_TAG_FEFLOOD MyHTML_TAG_FEFUNCA MyHTML_TAG_FEFUNCB MyHTML_TAG_FEFUNCG MyHTML_TAG_FEFUNCR
		MyHTML_TAG_FEGAUSSIANBLUR MyHTML_TAG_FEIMAGE MyHTML_TAG_FEMERGE MyHTML_TAG_FEMERGENODE MyHTML_TAG_FEMORPHOLOGY
		MyHTML_TAG_FEOFFSET MyHTML_TAG_FEPOINTLIGHT MyHTML_TAG_FESPECULARLIGHTING MyHTML_TAG_FESPOTLIGHT MyHTML_TAG_FETILE
		MyHTML_TAG_FETURBULENCE MyHTML_TAG_FILTER MyHTML_TAG_FONT_FACE MyHTML_TAG_FONT_FACE_FORMAT MyHTML_TAG_FONT_FACE_NAME
		MyHTML_TAG_FONT_FACE_SRC MyHTML_TAG_FONT_FACE_URI MyHTML_TAG_FOREIGNOBJECT MyHTML_TAG_G MyHTML_TAG_GLYPH MyHTML_TAG_GLYPHREF
		MyHTML_TAG_HKERN MyHTML_TAG_LINE MyHTML_TAG_LINEARGRADIENT MyHTML_TAG_MARKER MyHTML_TAG_MASK MyHTML_TAG_METADATA
		MyHTML_TAG_MISSING_GLYPH MyHTML_TAG_MPATH MyHTML_TAG_PATH MyHTML_TAG_PATTERN MyHTML_TAG_POLYGON MyHTML_TAG_POLYLINE
		MyHTML_TAG_RADIALGRADIENT MyHTML_TAG_RECT MyHTML_TAG_SET MyHTML_TAG_STOP MyHTML_TAG_SWITCH MyHTML_TAG_SYMBOL
		MyHTML_TAG_TEXT MyHTML_TAG_TEXTPATH MyHTML_TAG_TREF MyHTML_TAG_TSPAN MyHTML_TAG_USE MyHTML_TAG_VIEW MyHTML_TAG_VKERN
		MyHTML_TAG_MATH MyHTML_TAG_MACTION MyHTML_TAG_MALIGNGROUP MyHTML_TAG_MALIGNMARK MyHTML_TAG_MENCLOSE MyHTML_TAG_MERROR
		MyHTML_TAG_MFENCED MyHTML_TAG_MFRAC MyHTML_TAG_MGLYPH MyHTML_TAG_MI MyHTML_TAG_MLABELEDTR MyHTML_TAG_MLONGDIV
		MyHTML_TAG_MMULTISCRIPTS MyHTML_TAG_MN MyHTML_TAG_MO MyHTML_TAG_MOVER MyHTML_TAG_MPADDED MyHTML_TAG_MPHANTOM
		MyHTML_TAG_MROOT MyHTML_TAG_MROW MyHTML_TAG_MS MyHTML_TAG_MSCARRIES MyHTML_TAG_MSCARRY MyHTML_TAG_MSGROUP
		MyHTML_TAG_MSLINE MyHTML_TAG_MSPACE MyHTML_TAG_MSQRT MyHTML_TAG_MSROW MyHTML_TAG_MSTACK MyHTML_TAG_MSTYLE
		MyHTML_TAG_MSUB MyHTML_TAG_MSUP MyHTML_TAG_MSUBSUP MyHTML_TAG__END_OF_FILE MyHTML_TAG_FIRST_ENTRY MyHTML_TAG_LAST_ENTRY
		
		MyHTML_NAMESPACE_UNDEF MyHTML_NAMESPACE_HTML MyHTML_NAMESPACE_MATHML MyHTML_NAMESPACE_SVG MyHTML_NAMESPACE_XLINK MyHTML_NAMESPACE_XML
		MyHTML_NAMESPACE_XMLNS MyHTML_NAMESPACE_LAST_ENTRY 
		
		MyHTML_STATUS_OK MyHTML_STATUS_ERROR_MEMORY_ALLOCATION MyHTML_STATUS_THREAD_ERROR_LIST_INIT MyHTML_STATUS_THREAD_ERROR_ATTR_MALLOC
		MyHTML_STATUS_THREAD_ERROR_ATTR_INIT MyHTML_STATUS_THREAD_ERROR_ATTR_SET MyHTML_STATUS_THREAD_ERROR_ATTR_DESTROY
		MyHTML_STATUS_THREAD_ERROR_NO_SLOTS MyHTML_STATUS_THREAD_ERROR_BATCH_INIT MyHTML_STATUS_THREAD_ERROR_WORKER_MALLOC
		MyHTML_STATUS_THREAD_ERROR_WORKER_SEM_CREATE MyHTML_STATUS_THREAD_ERROR_WORKER_THREAD_CREATE MyHTML_STATUS_THREAD_ERROR_MASTER_THREAD_CREATE
		MyHTML_STATUS_THREAD_ERROR_SEM_PREFIX_MALLOC MyHTML_STATUS_THREAD_ERROR_SEM_CREATE MyHTML_STATUS_THREAD_ERROR_QUEUE_MALLOC
		MyHTML_STATUS_THREAD_ERROR_QUEUE_NODES_MALLOC MyHTML_STATUS_THREAD_ERROR_QUEUE_NODE_MALLOC MyHTML_STATUS_RULES_ERROR_MEMORY_ALLOCATION
		MyHTML_STATUS_PERF_ERROR_COMPILED_WITHOUT_PERF MyHTML_STATUS_PERF_ERROR_FIND_CPU_CLOCK MyHTML_STATUS_TOKENIZER_ERROR_MEMORY_ALLOCATION
		MyHTML_STATUS_TAGS_ERROR_MEMORY_ALLOCATION MyHTML_STATUS_TAGS_ERROR_MCOBJECT_CREATE MyHTML_STATUS_TAGS_ERROR_MCOBJECT_MALLOC
		MyHTML_STATUS_TAGS_ERROR_MCOBJECT_CREATE_NODE MyHTML_STATUS_TAGS_ERROR_CACHE_MEMORY_ALLOCATION MyHTML_STATUS_TAGS_ERROR_INDEX_MEMORY_ALLOCATION
		MyHTML_STATUS_TREE_ERROR_MEMORY_ALLOCATION MyHTML_STATUS_TREE_ERROR_MCOBJECT_CREATE MyHTML_STATUS_TREE_ERROR_MCOBJECT_INIT
		MyHTML_STATUS_TREE_ERROR_MCOBJECT_CREATE_NODE MyHTML_STATUS_ATTR_ERROR_ALLOCATION MyHTML_STATUS_ATTR_ERROR_CREATE
		
		MyHTML_OPTIONS_DEFAULT MyHTML_OPTIONS_PARSE_MODE_SINGLE MyHTML_OPTIONS_PARSE_MODE_ALL_IN_ONE MyHTML_OPTIONS_PARSE_MODE_SEPARATELY
		MyHTML_OPTIONS_PARSE_MODE_WORKER_TREE MyHTML_OPTIONS_PARSE_MODE_WORKER_INDEX MyHTML_OPTIONS_PARSE_MODE_TREE_INDEX 
	);
};

bootstrap HTML::MyHTML $VERSION;

use DynaLoader ();
use Exporter ();

1;


__END__

=head1 NAME

HTML::MyHTML is a fast HTML Parser using Threads with no outside dependencies

=head1 DESCRIPTION

This Parser based on L<MyHTML library|https://github.com/lexborisov/myhtml>  (it includes version 0.4.5)

=over 4

=item * Asynchronous Parsing, Build Tree and Indexation

=item * Fully conformant with the L<HTML5 specification|https://html.spec.whatwg.org/multipage/>

=item * Manipulation of elements: add, change, delete and other (available in C lib, in Perl coming soon)

=item * Manipulation of elements attributes: add, change, delete and other (available in C lib, in Perl coming soon)

=item * Support 34 character encoding by specification L<encoding.spec.whatwg.org|https://encoding.spec.whatwg.org/>

=item * Support detecting character encodings

=item * Support Single Mode parsing

=item * Support for fragment parsing

=item * Support for parsing by chunks

=item * No outside dependencies

=item * Passes all tree construction tests from L<html5lib-tests|https://github.com/html5lib/html5lib-tests>

See latest version on L<https://github.com/lexborisov/perl-html-myhtml|https://github.com/lexborisov/perl-html-myhtml>

=back

=head1 SYNOPSIS

 
 use utf8;
 use strict;
 use HTML::MyHTML;
 
 my $body = "<div><span>Best of Fragments</span><a>click to make happy</a></div><div some=value></div>";
 
 # init
 my $myhtml = HTML::MyHTML->new(MyHTML_OPTIONS_DEFAULT, 1);
 my $tree = $myhtml->new_tree();
 
 # parse
 $myhtml->parse($tree, MyHTML_ENCODING_UTF_8, $body);
 
 # print result
 print "Print HTML Tree:\n";
 $tree->document->print_children($tree, *STDOUT, 0);
 
 print "\nGet all DIV elements of HTML Tree:\n";
 my $list = $tree->get_elements_by_tag_name("div");
 
 foreach my $node (@$list) {
 	my $info = $node->info($tree);
 	
 	print "Tag id: ", $info->{tag_id}, "\n";
 	print "Tag name: ", $info->{tag}, "\n";
 	print "Namespace: ", $info->{namespace}, "\n";
 	print "Namespace id: ", $info->{namespace_id}, "\n";
 	
 	my $attr = $info->{attr};
 	if (keys %$attr) {
 		print "Attributes: \n";
 		foreach my $key (keys %$attr) {
 			print "\t", "$key=\"", $attr->{$key}, "\"\n";
 		}
 	}
 	
 	print "\n";
 }
 
 # or you can get span
 # tree -> document -> HTML -> BODY -> DIV -> SPAN
 my $span = $tree->document->child->last_child->child->child;
 my $info_of_span = $span->info($tree);
 
 $tree->destroy();
 

=head1 Methods

=head2 MyHTML

=head3 new

Create a MyHTML object. Allocating and Initialization resources for a MyHTML object

 # $opt[in] work options, how many threads will be. Default: MyHTML_OPTIONS_PARSE_MODE_SEPARATELY
 # $thread_count[in] thread count, it depends on the choice of work options. Default: 1
 # $out_status[out] status
 
 my $myhtml = HTML::MyHTML->new($opt, $thread_count, $out_status);

Return: HTML::MyHTML if successful, otherwise a UNDEF value


=head3 new_tree

Create a MyHTML::TREE object. Allocating and Initialization resources for a MyHTML::TREE object

 my $tree = $myhtml->new_tree($out_status);

Return: MyHTML::TREE object if successful, otherwise a UNDEF value


=head3 parse

Parsing HTML

 # $tree[in] previously created object MyHTML::TREE
 # $encoding[in] Input character encoding; Default: MyHTML_ENCODING_UTF_8 or MyHTML_ENCODING_DEFAULT or 0
 # $html[in] HTML
 
 my $status = $myhtml->parse($tree, $encoding, $html);

Return: MyHTML_STATUS_OK if successful, otherwise an error status


=head3 parse_fragment

Parsing fragment of HTML

 # $tree[in] previously created object MyHTML::TREE
 # $encoding[in] Input character encoding; Default: MyHTML_ENCODING_UTF_8 or MyHTML_ENCODING_DEFAULT or 0
 # $html[in] HTML
 # $tag_id[in] fragment base (root) tag id. Default: MyHTML_TAG_DIV if set 0
 # $my_namespace[in] fragment NAMESPACE. Default: MyHTML_NAMESPACE_HTML if set 0
 
 my $status = $myhtml->parse_fragment($tree, $encoding, $html, $tag_id, $my_namespace);

Return: MyHTML_STATUS_OK if successful, otherwise an error status


=head3 parse_single

Parsing HTML in Single Mode. No matter what was said during initialization MyHTML

 # $tree[in] previously created object MyHTML::TREE
 # $encoding[in] Input character encoding; Default: MyHTML_ENCODING_UTF_8 or MyHTML_ENCODING_DEFAULT or 0
 # $html[in] HTML
 
 my $status = $myhtml->parse_single($tree, $encoding, $html);

Return: MyHTML_STATUS_OK if successful, otherwise an error status


=head3 parse_fragment_single

Parsing fragment of HTML in Single Mode.  No matter what was said during initialization MyHTML

 # $tree[in] previously created object MyHTML::TREE
 # $encoding[in] Input character encoding; Default: MyHTML_ENCODING_UTF_8 or MyHTML_ENCODING_DEFAULT or 0
 # $html[in] HTML
 # $tag_id[in] fragment base (root) tag id. Default: MyHTML_TAG_DIV if set 0
 # $my_namespace[in] fragment NAMESPACE. Default: MyHTML_NAMESPACE_HTML if set 0
 
 my $status = $myhtml->parse_fragment_single($tree, $encoding, $html, $tag_id, $my_namespace);

Return: MyHTML_STATUS_OK if successful, otherwise an error status


=head3 parse_chunk

Parsing HTML chunk. For end parsing call parse_chunk_end method

 my $status = $myhtml->parse_chunk($tree, $html);

Return: MyHTML_STATUS_OK if successful, otherwise an error status


=head3 parse_chunk_fragment

Parsing chunk of fragment HTML. For end parsing call parse_chunk_end method

 my $status = $myhtml->parse_chunk_fragment($tree, $html, $tag_id, $my_namespace);

Return: MyHTML_STATUS_OK if successful, otherwise an error status


=head3 parse_chunk_single

Parsing HTML chunk in Single Mode.

 my $status = $myhtml->parse_chunk_single($tree, $html);

Return: MyHTML_STATUS_OK if successful, otherwise an error status


=head3 parse_chunk_fragment_single

Parsing chunk of fragment of HTML in Single Mode. No matter what was said during initialization MyHTML

 my $status = $myhtml->parse_chunk_fragment_single($tree, $html, $tag_id, $my_namespace);

Return: MyHTML_STATUS_OK if successful, otherwise an error status


=head3 parse_chunk_end

End of parsing HTML chunks

 my $status = $myhtml->parse_chunk_end($tree);

Return: MyHTML_STATUS_OK if successful, otherwise an error status


=head3 get_tag

Get HTML::MyHTML::Tag from a HTML::MyHTML

 my $tag = $myhtml->get_tag();

Return: HTML::MyHTML::Tag if exists, otherwise a UNDEF value


=head2 Tree

=head3 clean

Clears resources before new parsing

 $tree->clean();

=head3 destroy

Destroy of a MyHTML_TREE structure

 my $tree = $tree->destroy();

Return: UNDEF if successful, otherwise an HTML::MyHTML::Tree structure


=head3 get_myhtml

Get HTML::MyHTML from a HTML::MyHTML::Tree object

 my $myhtml = $tree->get_myhtml();

Return: HTML::MyHTML if exists, otherwise a UNDEF value


=head3 get_tag

Get HTML::MyHTML::Tag from a HTML::MyHTML::Tree

 my $tag = $tree->get_tag();

Return: HTML::MyHTML::Tag if exists, otherwise a UNDEF value


=head3 get_tag_index

Get HTML::MyHTML::Tag from a HTML::MyHTML::Tree

 my $tag_index = $tree->get_tag_index();

Return: HTML::MyHTML::Tag::Index if exists, otherwise a UNDEF value


=head3 document

Get Tree Document (Root of Tree)

 my $node = $tree->document();

Return: HTML::MyHTML::Tree::Node if successful, otherwise a UNDEF value


=head3 get_mchar

 my $mchar_async_t = $tree->get_mchar();

Return: mchar_async_t* if exists, otherwise a UNDEF value


=head3 get_mchar_node_id

 my $id = $tree->get_mchar_node_id();

Return: node id


=head3 get_elements_by_tag_id

 my $res = $tree->get_elements_by_tag_id($tag_id);

Return: array list of elements HTML::MyHTML::Tree::Node


=head3 get_elements_by_tag_name

 my $res = $tree->get_elements_by_tag_name($tag_name);

Return: array list of elements HTML::MyHTML::Tree::Node


=head2 Attributes

=head3 info

Get information of attribute: key, value, namespace

 my $res = $attr->info();

Return: hash ref 


=head3 name

Get attribute name (key)

 my $res = $attr->name();

Return: name (key) if exists, otherwise an UNDEF value


=head3 value

Get attribute value

 my $res = $attr->value();

Return: value if exists, otherwise an UNDEF value


=head3 next

Get next sibling attribute of one node

 my $attr = $attr->next();

Return: HTML::MyHTML::Tree::Attr if exists, otherwise an UNDEF value


=head3 prev

Get previous sibling attribute of one node

 my $attr = $attr->prev();

Return: HTML::MyHTML::Tree::Attr if exists, otherwise an UNDEF value


=head3 namespace

Get attribute namespace

 my $namespace = $attr->namespace();

Return: namespace id


=head3 remove

Remove attribute reference. Do not release the resources

 my $attr = $attr->remove($node);

Return: HTML::MyHTML::Tree::Attr if successful, otherwise a UNDEF value

=head3 delete

Remove attribute and release allocated resources

 $attr->delete($tree, $node);

=head3 free

Release allocated resources

 $attr->free($tree);


=head2 Node

=head3 info

Get information of node: tag name, tag id, namespace, namespace id, attr

 my $res = $node->info($tree);

Return: hash ref


=head3 next

Get next sibling node

 my $node = $node->next();

Return: HTML::MyHTML::Tree::Node if exists, otherwise an UNDEF value


=head3 prev

Get previous sibling node

 my $node = $node->prev();

Return: HTML::MyHTML::Tree::Node if exists, otherwise an UNDEF value


=head3 parent

Get parent node

 my $node = $node->parent();

Return: HTML::MyHTML::Tree::Node if exists, otherwise an UNDEF value


=head3 child

Get child (first child) of node

 my $node = $node->child();

Return: HTML::MyHTML::Tree::Node if exists, otherwise an UNDEF value


=head3 last_child

Get last child of node

 my $node = $node->last_child();

Return: HTML::MyHTML::Tree::Node if exists, otherwise an UNDEF value


=head3 free

Release allocated resources

 $node->free($node, $tree);

=head3 remove

Remove node of tree

 my $node = $node->remove();

Return: HTML::MyHTML::Tree::Node if successful, otherwise a UNDEF value


=head3 delete

Remove node of tree and release allocated resources

 $node->delete($tree);

=head3 delete_recursive

Remove nodes of tree recursively and release allocated resources

 $node->delete_recursive($tree);

=head3 tag_id

Get node tag id

 my $tag_id = $node->tag_id();

Return: tag_id


=head3 namespace

Get node namespace

 my $namespace = $node->namespace();

Return: namespace id


=head3 tag_name

Get tag name of a node

 my $res = $node->tag_name($tree);

Return: tag name


=head3 is_close_self

Node has self-closing flag?

 my $bool = $node->is_close_self();

Return: 1 (true) or 0 (false)


=head3 attr_first

Get first attribute of a node

 my $attr = $node->attr_first();

Return: HTML::MyHTML::Tree::Attr if exists, otherwise an UNDEF value


=head3 attr_last

Get last attribute of a node

 my $attr = $node->attr_last();

Return: HTML::MyHTML::Tree::Attr if exists, otherwise an UNDEF value


=head3 attr_add

Add attribute to tree node

 my $attr = $node->attr_add($tree, $key, $value, $encoding);

Return: HTML::MyHTML::Tree::Attr if successful, otherwise an UNDEF value


=head3 attr_remove_by_key

Remove attribute by key reference. Do not release the resources

 my $attr = $node->attr_remove_by_key($key);

Return: HTML::MyHTML::Tree::Attr if successful, otherwise an UNDEF value


=head3 attr_by_key

Get attribute by key

 my $attr = $node->attr_by_key($key);

Return: HTML::MyHTML::Tree::Attr if exists, otherwise an UNDEF value


=head3 text

Get text of a node. Only for a MyHTML_TAG__TEXT or MyHTML_TAG__COMMENT tags

 my $res = $node->text();

Return: text if exists, otherwise an UNDEF value


=head3 string

Get myhtml_string_t object by Tree node

 my $string = $node->string();

Return: HTML::MyHTML::String if exists, otherwise an NULL value


=head3 print

Print a node

 $node->print($tree, $fh, $inc);

=head3 print_children

Print tree of a node. Print excluding current node

 $node->print_children($tree, $fh, $inc);

=head3 print_all

Print tree of a node. Print including current node

 $node->print_all($tree, $fh);


=head2 Detect encoding

=head3 encoding_detect

Detect character encoding.

Now available for detect UTF-8, UTF-16LE, UTF-16BE and Russians: windows-1251,  koi8-r, iso-8859-5, x-mac-cyrillic, ibm866. Other in progress

 # $text[in] text
 # $out_encoding[out] detected encoding
 
 my $bool = $myhtml->encoding_detect($text, $out_encoding);

Return: 1 (true) if encoding found, otherwise 0 (false)

=head3 encoding_detect_russian

Detect Russian character encoding

Now available for detect windows-1251,  koi8-r, iso-8859-5, x-mac-cyrillic, ibm866

 # $text[in] text
 # $out_encoding[out] detected encoding
 
 my $bool = $myhtml->encoding_detect_russian($text, $out_encoding);

Return: 1 (true) if encoding found, otherwise 0 (false)

=head3 encoding_detect_unicode

Detect Unicode character encoding

Now available for detect UTF-8, UTF-16LE, UTF-16BE

 # $text[in] text
 # $out_encoding[out] detected encoding
 
 my $bool = $myhtml->encoding_detect_unicode($text, $out_encoding);

Return: 1 (true) if encoding found, otherwise 0 (false)

=head3 encoding_detect_bom

Detect Unicode character encoding by BOM

Now available for detect UTF-8, UTF-16LE, UTF-16BE

 # $text[in] text
 # $out_encoding[out] detected encoding
 
 my $bool = $myhtml->encoding_detect_bom($text, $out_encoding);

Return: 1 (true) if encoding found, otherwise 0 (false)


=head1 Constants

=head2 Tags

 MyHTML_TAG__UNDEF
 MyHTML_TAG__TEXT
 MyHTML_TAG__COMMENT
 MyHTML_TAG__DOCTYPE
 MyHTML_TAG_A
 MyHTML_TAG_ABBR
 MyHTML_TAG_ACRONYM
 MyHTML_TAG_ADDRESS
 MyHTML_TAG_ANNOTATION_XML
 MyHTML_TAG_APPLET
 MyHTML_TAG_AREA
 MyHTML_TAG_ARTICLE
 MyHTML_TAG_ASIDE
 MyHTML_TAG_AUDIO
 MyHTML_TAG_B
 MyHTML_TAG_BASE
 MyHTML_TAG_BASEFONT
 MyHTML_TAG_BDI
 MyHTML_TAG_BDO
 MyHTML_TAG_BGSOUND
 MyHTML_TAG_BIG
 MyHTML_TAG_BLINK
 MyHTML_TAG_BLOCKQUOTE
 MyHTML_TAG_BODY
 MyHTML_TAG_BR
 MyHTML_TAG_BUTTON
 MyHTML_TAG_CANVAS
 MyHTML_TAG_CAPTION
 MyHTML_TAG_CENTER
 MyHTML_TAG_CITE
 MyHTML_TAG_CODE
 MyHTML_TAG_COL
 MyHTML_TAG_COLGROUP
 MyHTML_TAG_COMMAND
 MyHTML_TAG_COMMENT
 MyHTML_TAG_DATALIST
 MyHTML_TAG_DD
 MyHTML_TAG_DEL
 MyHTML_TAG_DETAILS
 MyHTML_TAG_DFN
 MyHTML_TAG_DIALOG
 MyHTML_TAG_DIR
 MyHTML_TAG_DIV
 MyHTML_TAG_DL
 MyHTML_TAG_DT
 MyHTML_TAG_EM
 MyHTML_TAG_EMBED
 MyHTML_TAG_FIELDSET
 MyHTML_TAG_FIGCAPTION
 MyHTML_TAG_FIGURE
 MyHTML_TAG_FONT
 MyHTML_TAG_FOOTER
 MyHTML_TAG_FORM
 MyHTML_TAG_FRAME
 MyHTML_TAG_FRAMESET
 MyHTML_TAG_H1
 MyHTML_TAG_H2
 MyHTML_TAG_H3
 MyHTML_TAG_H4
 MyHTML_TAG_H5
 MyHTML_TAG_H6
 MyHTML_TAG_HEAD
 MyHTML_TAG_HEADER
 MyHTML_TAG_HGROUP
 MyHTML_TAG_HR
 MyHTML_TAG_HTML
 MyHTML_TAG_I
 MyHTML_TAG_IFRAME
 MyHTML_TAG_IMAGE
 MyHTML_TAG_IMG
 MyHTML_TAG_INPUT
 MyHTML_TAG_INS
 MyHTML_TAG_ISINDEX
 MyHTML_TAG_KBD
 MyHTML_TAG_KEYGEN
 MyHTML_TAG_LABEL
 MyHTML_TAG_LEGEND
 MyHTML_TAG_LI
 MyHTML_TAG_LINK
 MyHTML_TAG_LISTING
 MyHTML_TAG_MAIN
 MyHTML_TAG_MAP
 MyHTML_TAG_MARK
 MyHTML_TAG_MARQUEE
 MyHTML_TAG_MENU
 MyHTML_TAG_MENUITEM
 MyHTML_TAG_META
 MyHTML_TAG_METER
 MyHTML_TAG_MTEXT
 MyHTML_TAG_NAV
 MyHTML_TAG_NOBR
 MyHTML_TAG_NOEMBED
 MyHTML_TAG_NOFRAMES
 MyHTML_TAG_NOSCRIPT
 MyHTML_TAG_OBJECT
 MyHTML_TAG_OL
 MyHTML_TAG_OPTGROUP
 MyHTML_TAG_OPTION
 MyHTML_TAG_OUTPUT
 MyHTML_TAG_P
 MyHTML_TAG_PARAM
 MyHTML_TAG_PLAINTEXT
 MyHTML_TAG_PRE
 MyHTML_TAG_PROGRESS
 MyHTML_TAG_Q
 MyHTML_TAG_RB
 MyHTML_TAG_RP
 MyHTML_TAG_RT
 MyHTML_TAG_RTC
 MyHTML_TAG_RUBY
 MyHTML_TAG_S
 MyHTML_TAG_SAMP
 MyHTML_TAG_SCRIPT
 MyHTML_TAG_SECTION
 MyHTML_TAG_SELECT
 MyHTML_TAG_SMALL
 MyHTML_TAG_SOURCE
 MyHTML_TAG_SPAN
 MyHTML_TAG_STRIKE
 MyHTML_TAG_STRONG
 MyHTML_TAG_STYLE
 MyHTML_TAG_SUB
 MyHTML_TAG_SUMMARY
 MyHTML_TAG_SUP
 MyHTML_TAG_SVG
 MyHTML_TAG_TABLE
 MyHTML_TAG_TBODY
 MyHTML_TAG_TD
 MyHTML_TAG_TEMPLATE
 MyHTML_TAG_TEXTAREA
 MyHTML_TAG_TFOOT
 MyHTML_TAG_TH
 MyHTML_TAG_THEAD
 MyHTML_TAG_TIME
 MyHTML_TAG_TITLE
 MyHTML_TAG_TR
 MyHTML_TAG_TRACK
 MyHTML_TAG_TT
 MyHTML_TAG_U
 MyHTML_TAG_UL
 MyHTML_TAG_VAR
 MyHTML_TAG_VIDEO
 MyHTML_TAG_WBR
 MyHTML_TAG_XMP
 MyHTML_TAG_ALTGLYPH
 MyHTML_TAG_ALTGLYPHDEF
 MyHTML_TAG_ALTGLYPHITEM
 MyHTML_TAG_ANIMATE
 MyHTML_TAG_ANIMATECOLOR
 MyHTML_TAG_ANIMATEMOTION
 MyHTML_TAG_ANIMATETRANSFORM
 MyHTML_TAG_CIRCLE
 MyHTML_TAG_CLIPPATH
 MyHTML_TAG_COLOR_PROFILE
 MyHTML_TAG_CURSOR
 MyHTML_TAG_DEFS
 MyHTML_TAG_DESC
 MyHTML_TAG_ELLIPSE
 MyHTML_TAG_FEBLEND
 MyHTML_TAG_FECOLORMATRIX
 MyHTML_TAG_FECOMPONENTTRANSFER
 MyHTML_TAG_FECOMPOSITE
 MyHTML_TAG_FECONVOLVEMATRIX
 MyHTML_TAG_FEDIFFUSELIGHTING
 MyHTML_TAG_FEDISPLACEMENTMAP
 MyHTML_TAG_FEDISTANTLIGHT
 MyHTML_TAG_FEDROPSHADOW
 MyHTML_TAG_FEFLOOD
 MyHTML_TAG_FEFUNCA
 MyHTML_TAG_FEFUNCB
 MyHTML_TAG_FEFUNCG
 MyHTML_TAG_FEFUNCR
 MyHTML_TAG_FEGAUSSIANBLUR
 MyHTML_TAG_FEIMAGE
 MyHTML_TAG_FEMERGE
 MyHTML_TAG_FEMERGENODE
 MyHTML_TAG_FEMORPHOLOGY
 MyHTML_TAG_FEOFFSET
 MyHTML_TAG_FEPOINTLIGHT
 MyHTML_TAG_FESPECULARLIGHTING
 MyHTML_TAG_FESPOTLIGHT
 MyHTML_TAG_FETILE
 MyHTML_TAG_FETURBULENCE
 MyHTML_TAG_FILTER
 MyHTML_TAG_FONT_FACE
 MyHTML_TAG_FONT_FACE_FORMAT
 MyHTML_TAG_FONT_FACE_NAME
 MyHTML_TAG_FONT_FACE_SRC
 MyHTML_TAG_FONT_FACE_URI
 MyHTML_TAG_FOREIGNOBJECT
 MyHTML_TAG_G
 MyHTML_TAG_GLYPH
 MyHTML_TAG_GLYPHREF
 MyHTML_TAG_HKERN
 MyHTML_TAG_LINE
 MyHTML_TAG_LINEARGRADIENT
 MyHTML_TAG_MARKER
 MyHTML_TAG_MASK
 MyHTML_TAG_METADATA
 MyHTML_TAG_MISSING_GLYPH
 MyHTML_TAG_MPATH
 MyHTML_TAG_PATH
 MyHTML_TAG_PATTERN
 MyHTML_TAG_POLYGON
 MyHTML_TAG_POLYLINE
 MyHTML_TAG_RADIALGRADIENT
 MyHTML_TAG_RECT
 MyHTML_TAG_SET
 MyHTML_TAG_STOP
 MyHTML_TAG_SWITCH
 MyHTML_TAG_SYMBOL
 MyHTML_TAG_TEXT
 MyHTML_TAG_TEXTPATH
 MyHTML_TAG_TREF
 MyHTML_TAG_TSPAN
 MyHTML_TAG_USE
 MyHTML_TAG_VIEW
 MyHTML_TAG_VKERN
 MyHTML_TAG_MATH
 MyHTML_TAG_MACTION
 MyHTML_TAG_MALIGNGROUP
 MyHTML_TAG_MALIGNMARK
 MyHTML_TAG_MENCLOSE
 MyHTML_TAG_MERROR
 MyHTML_TAG_MFENCED
 MyHTML_TAG_MFRAC
 MyHTML_TAG_MGLYPH
 MyHTML_TAG_MI
 MyHTML_TAG_MLABELEDTR
 MyHTML_TAG_MLONGDIV
 MyHTML_TAG_MMULTISCRIPTS
 MyHTML_TAG_MN
 MyHTML_TAG_MO
 MyHTML_TAG_MOVER
 MyHTML_TAG_MPADDED
 MyHTML_TAG_MPHANTOM
 MyHTML_TAG_MROOT
 MyHTML_TAG_MROW
 MyHTML_TAG_MS
 MyHTML_TAG_MSCARRIES
 MyHTML_TAG_MSCARRY
 MyHTML_TAG_MSGROUP
 MyHTML_TAG_MSLINE
 MyHTML_TAG_MSPACE
 MyHTML_TAG_MSQRT
 MyHTML_TAG_MSROW
 MyHTML_TAG_MSTACK
 MyHTML_TAG_MSTYLE
 MyHTML_TAG_MSUB
 MyHTML_TAG_MSUP
 MyHTML_TAG_MSUBSUP
 MyHTML_TAG__END_OF_FILE
 MyHTML_TAG_FIRST_ENTRY
 MyHTML_TAG_LAST_ENTRY


=head2 Namespaces

 MyHTML_NAMESPACE_UNDEF
 MyHTML_NAMESPACE_HTML
 MyHTML_NAMESPACE_MATHML
 MyHTML_NAMESPACE_SVG
 MyHTML_NAMESPACE_XLINK
 MyHTML_NAMESPACE_XML
 MyHTML_NAMESPACE_XMLNS
 MyHTML_NAMESPACE_LAST_ENTRY


=head2 Encodings

 MyHTML_ENCODING_DEFAULT
 MyHTML_ENCODING_UTF_8
 MyHTML_ENCODING_UTF_16LE
 MyHTML_ENCODING_UTF_16BE
 MyHTML_ENCODING_X_USER_DEFINED
 MyHTML_ENCODING_BIG5
 MyHTML_ENCODING_EUC_KR
 MyHTML_ENCODING_GB18030
 MyHTML_ENCODING_IBM866
 MyHTML_ENCODING_ISO_8859_10
 MyHTML_ENCODING_ISO_8859_13
 MyHTML_ENCODING_ISO_8859_14
 MyHTML_ENCODING_ISO_8859_15
 MyHTML_ENCODING_ISO_8859_16
 MyHTML_ENCODING_ISO_8859_2
 MyHTML_ENCODING_ISO_8859_3
 MyHTML_ENCODING_ISO_8859_4
 MyHTML_ENCODING_ISO_8859_5
 MyHTML_ENCODING_ISO_8859_6
 MyHTML_ENCODING_ISO_8859_7
 MyHTML_ENCODING_ISO_8859_8
 MyHTML_ENCODING_KOI8_R
 MyHTML_ENCODING_KOI8_U
 MyHTML_ENCODING_MACINTOSH
 MyHTML_ENCODING_WINDOWS_1250
 MyHTML_ENCODING_WINDOWS_1251
 MyHTML_ENCODING_WINDOWS_1252
 MyHTML_ENCODING_WINDOWS_1253
 MyHTML_ENCODING_WINDOWS_1254
 MyHTML_ENCODING_WINDOWS_1255
 MyHTML_ENCODING_WINDOWS_1256
 MyHTML_ENCODING_WINDOWS_1257
 MyHTML_ENCODING_WINDOWS_1258
 MyHTML_ENCODING_WINDOWS_874
 MyHTML_ENCODING_X_MAC_CYRILLIC
 MyHTML_ENCODING_LAST_ENTRY


=head2 Statuses

 MyHTML_STATUS_OK
 MyHTML_STATUS_ERROR_MEMORY_ALLOCATION
 MyHTML_STATUS_THREAD_ERROR_LIST_INIT
 MyHTML_STATUS_THREAD_ERROR_ATTR_MALLOC
 MyHTML_STATUS_THREAD_ERROR_ATTR_INIT
 MyHTML_STATUS_THREAD_ERROR_ATTR_SET
 MyHTML_STATUS_THREAD_ERROR_ATTR_DESTROY
 MyHTML_STATUS_THREAD_ERROR_NO_SLOTS
 MyHTML_STATUS_THREAD_ERROR_BATCH_INIT
 MyHTML_STATUS_THREAD_ERROR_WORKER_MALLOC
 MyHTML_STATUS_THREAD_ERROR_WORKER_SEM_CREATE
 MyHTML_STATUS_THREAD_ERROR_WORKER_THREAD_CREATE
 MyHTML_STATUS_THREAD_ERROR_MASTER_THREAD_CREATE
 MyHTML_STATUS_THREAD_ERROR_SEM_PREFIX_MALLOC
 MyHTML_STATUS_THREAD_ERROR_SEM_CREATE
 MyHTML_STATUS_THREAD_ERROR_QUEUE_MALLOC
 MyHTML_STATUS_THREAD_ERROR_QUEUE_NODES_MALLOC
 MyHTML_STATUS_THREAD_ERROR_QUEUE_NODE_MALLOC
 MyHTML_STATUS_RULES_ERROR_MEMORY_ALLOCATION
 MyHTML_STATUS_PERF_ERROR_COMPILED_WITHOUT_PERF
 MyHTML_STATUS_PERF_ERROR_FIND_CPU_CLOCK
 MyHTML_STATUS_TOKENIZER_ERROR_MEMORY_ALLOCATION
 MyHTML_STATUS_TAGS_ERROR_MEMORY_ALLOCATION
 MyHTML_STATUS_TAGS_ERROR_MCOBJECT_CREATE
 MyHTML_STATUS_TAGS_ERROR_MCOBJECT_MALLOC
 MyHTML_STATUS_TAGS_ERROR_MCOBJECT_CREATE_NODE
 MyHTML_STATUS_TAGS_ERROR_CACHE_MEMORY_ALLOCATION
 MyHTML_STATUS_TAGS_ERROR_INDEX_MEMORY_ALLOCATION
 MyHTML_STATUS_TREE_ERROR_MEMORY_ALLOCATION
 MyHTML_STATUS_TREE_ERROR_MCOBJECT_CREATE
 MyHTML_STATUS_TREE_ERROR_MCOBJECT_INIT
 MyHTML_STATUS_TREE_ERROR_MCOBJECT_CREATE_NODE
 MyHTML_STATUS_ATTR_ERROR_ALLOCATION
 MyHTML_STATUS_ATTR_ERROR_CREATE


=head2 Options

 MyHTML_OPTIONS_DEFAULT
 MyHTML_OPTIONS_PARSE_MODE_SINGLE
 MyHTML_OPTIONS_PARSE_MODE_ALL_IN_ONE
 MyHTML_OPTIONS_PARSE_MODE_SEPARATELY
 MyHTML_OPTIONS_PARSE_MODE_WORKER_TREE
 MyHTML_OPTIONS_PARSE_MODE_WORKER_INDEX
 MyHTML_OPTIONS_PARSE_MODE_TREE_INDEX


=head1 Examples

See example directory in current module


=head1 DESTROY

 undef $myhtml;

Free mem and destroy object.

=head1 AUTHOR

Alexander Borisov <lex.borisov@gmail.com>

=head1 COPYRIGHT AND LICENSE

Copyright 2015-2016 Alexander Borisov

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

=cut
