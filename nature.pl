#! /usr/bin/perl -w
# http://search.cpan.org/~jzucker/DBD-CSV-0.22/lib/DBD/CSV.pm
use DBI;
local $MAX_COUNT = 40;
local $curid;
local $prgname = "oygo_renju.exe";

my $time_to_die = 0;
sub signal_handler {
	$time_to_die = 1;
}
$SIG{INT} = $SIG{TERM} = $SIG{HUP} = \&signal_handler;

my $dbh = DBI->connect("DBI:CSV:f_dir=./csv");
$dbh->{'RaiseError'} = 1;
$@ = '';
eval {
	$dbh->do("CREATE TABLE renju (id INTEGER, gid INTEGER, life INTEGER, gene1 INTEGER, gene2 char(50), gene3 char(50))");
};
sub insert_rand {
	my $id = shift;
	my ($gene1, $gene2, $gene3) = rand_gene();
	return "INSERT INTO renju VALUES ($id, $id, 10, '$gene1', '$gene2', '$gene3')";
}
sub remove {
	my $id = shift;
	return "delete from renju where id=$id";
}
#mature [0,+infinite)
#explore_rate [0, 16) float
#aaf_fraction [0,1] float
sub rand_gene {
	return (int(rand 50000), rand 16, rand 1);
}
sub clone {
	my $a = shift;
	my ($id, $gid, $life, $gene1, $gene2, $gene3) = @$a;
	$curid++;
	if(int(rand 5) == 1) {
		my $n = int(rand 3);
		if($n == 0) {
			$gene1 = int(mutate($gene1, 0, 50000));
		} elsif($n == 1) {
			$gene2 = mutate($gene2, 0, 16);
		} else {
			$gene3 = mutate($gene3, 0, 1);
		}
		unless(($gene1 == $a->[3] && 
				$gene2 == $a->[4] && 
				$gene3 == $a->[5])) {
			$gid = $curid;
		}
	}
	return "INSERT INTO renju VALUES ($curid, $gid, 10, '$gene1', '$gene2', '$gene3')";
}
# <1>步长变异
# <2>高斯变异
# 这里用的是步长变异
sub mutate {
	my $value = shift;
	my $lower = shift;
	my $upper = shift;
	my $Boundary = 1/20;
	my $Delta = 0;
	for (my $i = 0; $i < 20; $i++) {
		if (rand 1 < $Boundary) {
			$Delta += 1/(1 << $i);
		}
	}
	if(rand 1 > 0.5) {
		$value += ($upper - $value) * $Delta * 0.5;
	} else {
		$value -= ($value - $lower) * $Delta * 0.5;
	}
	return $value;
}
sub match {
	my $a1 = shift;
	my $a2 = shift;
	my $gid1 = $a1->[1];
	my $gid2 = $a2->[1];
	my $str1 = join(",", (@$a1)[3,4,5]);
	my $str2 = join(",", (@$a2)[3,4,5]);
	#print "$str1 win $str2\n";
	my $cmd = "perl towgo.pl --games 2 -b='$prgname --arg=$str1' -w='$prgname --arg=$str2' 2>/dev/null";
	my $result = `$cmd`;
	if($result =~ /winner:(.*)$/i) {
		$winner = $1;
		if($winner eq "$prgname --arg=$str1") {
			print "$gid1 -> $gid2\n";
			return 1;
		} else {
			print "$gid2 -> $gid1\n";
			return -1;
		}
	}
	print "--$gid1,$gid2\n";
	return 0;
}
while(!$time_to_die) {

	my $count;
	$sth = $dbh->prepare("SELECT count(id) as n FROM renju");
	$sth->execute();
	$sth->bind_columns(undef, \$count);
	$sth->fetch;
	$sth->finish;

	$sth = $dbh->prepare("SELECT max(id) as n FROM renju");
	$sth->execute();
	$sth->bind_columns(undef, \$curid);
	$sth->fetch;
	$sth->finish;
	$curid = 0 unless($curid);
	$count = 0 unless($count);
	for(my $i = $count; $i < $MAX_COUNT; $i++) {
		$curid++;
		$dbh->do(insert_rand($curid));
	}

	my $randrow = int(rand($MAX_COUNT));
	$sth = $dbh->prepare("SELECT * FROM renju limit $randrow,1");
	$sth->execute();
	my @a1 = $sth->fetchrow();
	$sth->finish;
	my $gid1 = $a1[1];
	$sth = $dbh->prepare("SELECT count(id) as n FROM renju where gid <> $gid1");
	my $numrows;
	$sth->execute();
	$sth->bind_columns(undef, \$numrows);
	$sth->fetch;
	$sth->finish;
	die "selection finished!" unless($numrows);
	$randrow = int(rand($numrows));
	$sth = $dbh->prepare("SELECT * FROM renju where gid <> $gid1 limit $randrow,1");
	$sth->execute();
	my @a2 = $sth->fetchrow();
	my $result = match(\@a1, \@a2);
	if(!$time_to_die) {
		$a1[2]--;
		$a2[2]--;
		$dbh->do("update renju set life=".$a1[2]." where id=".$a1[0]);
		$dbh->do("update renju set life=".$a1[2]." where id=".$a2[0]);
	}
	if($result > 0) {
		$dbh->do(clone(\@a1));
		$dbh->do(remove($a2[0]));
	} elsif ($result < 0) {
		$dbh->do(clone(\@a2));
		$dbh->do(remove($a1[0]));
	} else {
	}
	$dbh->do("delete from renju where life<=0");

} # end while

$dbh->disconnect();
print "exit...\n";
