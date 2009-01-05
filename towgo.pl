#! /usr/bin/perl -w 

# 
# Run with:
# 
# twogtp --white '<path to program 1> --mode gtp <options>' \
#        --black '<path to program 2> --mode gtp <options>' \
#        [twogtp options]
#

package TWOGTP_A;

use IPC::Open2;
use IPC::Open3;
use Getopt::Long;
use FileHandle;
use strict;
use warnings;
use Carp;

STDOUT->autoflush(1);

#following added globally to allow "use strict" :
my $vertex;
my $first;
my $sgfmove;
my $sgffilename;
my $pidw;
my $pidb;
my $sgffile;
my $handicap_stones;
my $resultw;
my $resultb;
my @vertices;
my $second;
my %game_list;
#end of "use strict" repairs


my $white;
my $black;
my $size = 19;
my $verbose = 1;
my $komi;
my $handicap = 0;
my $games = 1;
my $wanthelp;

my $helpstring = "

Run with:

twogtp --white \'<path to program 1>\' \\
       --black \'<path to program 2>\' \\

  --help                                  (show this)

";

GetOptions(
           "white|w=s"              => \$white,
           "black|b=s"              => \$black,
           "verbose|v=i"            => \$verbose,
           "games=i"                => \$games,
           "help"                   => \$wanthelp,
);

if ($wanthelp) {
  print $helpstring;
  exit;
}

die $helpstring unless defined $white and defined $black;

# create FileHandles
#my $black_in;
my $black_in  = new FileHandle;		# stdin of black player
my $black_out = new FileHandle;		# stdout of black player
my $white_in  = new FileHandle;		# stdin of white player
my $white_out = new FileHandle;		# stdout of white player
my $err = new FileHandle;
my $b_gtp_ver;				# gtp version of black player
my $w_gtp_ver;				# gtp version of white player

while ($games > 0) {
	#print "B:$black\tW:$white\n" if $verbose;

    $pidb = open3($black_in, $black_out, ">&STDERR", $black);
    #print "black pid: $pidb\n" if $verbose;
    $pidw = open3($white_in, $white_out, ">&STDERR", $white." 1");
    #print "white pid: $pidw\n" if $verbose;
    
    my $pass = 0;
    my ($move, $toplay);
    my $winner = "";

    $toplay = 'B';
    while ($pass < 2) {
	if ($toplay eq 'B') {

	    $move = eat_move($black_out);
	    #print "Black plays $move\n" if $verbose;
	    if ($move =~ /PASS/i) {
		$pass++;
	    } else {
		$pass = 0;
	    }
	    if ($move =~ /Resign/i) {
		$winner = $white;
		last;
	    }
	    print $white_in "$move\n";
	    $toplay = 'W';
	} else {
	    $move = eat_move($white_out);
	    #print "White plays $move\n" if $verbose;
	    if ($move =~ /PASS/i) {
		$pass++;
	    } else {
		$pass = 0;
	    }
	    if ($move =~ /Resign/i) {
		$winner = $black;
		last;
	    }
	    print $black_in "$move\n";
	    $toplay = 'B';
	}
    }
    $game_list{$winner}++;
    if($winner == $black) { print "B "; }
    else { print "W "; }
    print "$winner win\n" if $verbose;

    $games-- if $games > 0;
    close $black_in;
    close $black_out;
    close $white_in;
    close $white_out;
    waitpid $pidb, 0;
    waitpid $pidw, 0;
    print "games remaining: $games\n";
    # swap
    my $temp = $black;
    $black = $white;
    $white = $temp;
}

 foreach my $key (keys %game_list) {
    print "$key:".$game_list{$key}."\n";
  }


sub eat_no_response {
    my $h = shift;

# ignore empty lines
    my $line = "";
    while ($line eq "") {
	chop($line = <$h>) or die "No response!";
        $line =~ s/(\s|\n)*$//smg;
    }
}

sub eat_move {
    my $h = shift;
# ignore empty lines
    my $line = "";
    while ($line eq "") {
	if (!defined($line = <$h>)) {
	    die "Engine crashed!\n";
	}
        $line =~ s/(\s|\n)*$//smg;
    }
    my ($equals, $move) = split('=', $line, 2);
    defined($move) or confess "no move found: line was: '$line'";
    return $move;
}

sub eat_handicap {
    my $h = shift;
    my $sgf_handicap = "AB";
# ignore empty lines, die if process is gone
    my $line = "";
    while ($line eq "") {
	chop($line = <$h>) or die "No response!";
    }
    @vertices = split(" ", $line);
    foreach $vertex (@vertices) {
	if (!($vertex eq "=")) {
	    $vertex = standard_to_sgf($vertex);
	    $sgf_handicap = "$sgf_handicap\[$vertex\]";
	}
    }		
    return "$sgf_handicap;";
}

sub eat_score {
    my $h = shift;
# ignore empty lines, die if process is gone
    my $line = "";
    while ($line eq "") {
	chop($line = <$h>) or die "No response!";
	$line =~ s/^\s*//msg;
	$line =~ s/\s*$//msg;
    }
    $line =~ s/\s*$//;
    my ($equals, $result) = split('=', $line, 2);
    $line = <$h>;
    return $result;
}

sub eat_gtp_ver {
    my $h = shift;
    my $line = "";

    while ($line eq "") {
	chop($line = <$h>) or die "No response!";
	$line =~ s/^\s*//msg;
	$line =~ s/\s*$//msg;
    }
    $line =~ s/\s*$//;
    my ($equals, $result) = split('=', $line, 2);
    $line = <$h>;
    return $result;
}

sub eat_showboard {
    my $h = shift;
    my $line = "";

    while ($line eq "") {
	chop($line = <$h>) or die "No response!";
	$line =~ s/^\s*//msg;
	$line =~ s/\s*$//msg;
    }
    $line =~ s/\s*$//;
    my ($equals, $result) = split('=', $line, 2);

    while (!($line =~ /^\s*$/)) {
	$result .= $line;
	$line = <$h>;
    }
    print STDERR $result;
}

sub standard_to_sgf {
    for (@_) { confess "Yikes!" if !defined($_); }
    for (@_) { tr/A-Z/a-z/ };
    $_ = shift(@_);
    /([a-z])([0-9]+)/;
    return "tt" if $_ eq "pass";
    
    $first = ord $1;
    if ($first > 104) {
	$first = $first - 1;
    }
    $first = chr($first);
    $second = chr($size+1-$2+96);
    return "$first$second";
}

sub rename_sgffile {
    my $nogames = int shift(@_);
    $_ = shift(@_);
    s/\.sgf$//;
    # Annoying to loose _001 on game #1 in multi-game set.
    # Could record as an additional parameter.
    # return "$_.sgf" if ($nogames == 1);
    return sprintf("$_" . "_%03d.sgf", $nogames);
}

sub index_name {
  $_ = shift;
  s/\.sgf$//;
  return $_ . "_index.html";
}
