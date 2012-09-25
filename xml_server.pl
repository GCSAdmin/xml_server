#!/usr/bin/perl

use strict;
use warnings;
use XML::Smart;
use IO::Socket;
use DBI;
use Parallel::ForkManager;

my $DEBUG=1;
my $rundir = '/home/dba/CBS/assistant_tools/xml_server';
my $LOG_FILE = $rundir."/xml_server.log";
my $MAX_JOB_PROCESS = 800;

chdir $rundir;

my $port = 33060;
my $localaddr = '172.23.16.20';
my $listen_socket = IO::Socket::INET->new(Listen => 800,
                 LocalAddr => $localaddr,
                 LocalPort => $port,
                 Timeout => 60*3,
                 Reuse => 1)
    or die "Can't Create listening socket: $!\n";

send_log("Starting xml receiver server on port $port...\n");

my $pm_job = new Parallel::ForkManager( $MAX_JOB_PROCESS );
my $socket_handle_counter=0;
my $max_accect_counter=100;
while(1) {
    #$pm_job->wait_children;
    next unless my $session = $listen_socket->accept;
    if( ++$socket_handle_counter >= $max_accect_counter) {
        $socket_handle_counter = 0;
        send_log("have accept $max_accect_counter socket." );
    }

    my $i=1;
    while($i > 0) {
        $i--;
        $pm_job->start and next;
        #my $start_secs=time();
        $listen_socket->close or die "[EMERG] can't close listen socket\n";
        my $remote_addr=$session->peerhost() . '_' . $session->peerport();
        #my $remote_desc="host: $remote_addr";
                my $xml_str;
        while (<$session>) {
            last if ($_ =~ /XML_TRANSFER_END/);
            $xml_str .= $_;
        }
        print $session 'XML_RECEIVE_SUCCESS';
        $session->close or die "[EMERG] can't close established socket!\n";
        #my $cost_secs = time() - $start_secs;
        # send_log($remote_desc . " SESSION PROCESS COST: $cost_secs" );
        #warn($xml_str);
        write_to_file(\$xml_str,$remote_addr);
        $pm_job->finish;
    }
    $session->close or die "[EMERG] can't close established socket!\n";
    #$pm_job->wait_children;
}

sub write_to_file {
    my $xml_str_p = shift;
    my $remote_addr=shift;
    open(OUTPUT,">xml_tmp/$remote_addr") || die "write_to_file: open $remote_addr failed";
    print OUTPUT $$xml_str_p;
    close(OUTPUT) || die "write_to_file: close $remote_addr failed";
    #if( $remote_addr =~ /10.160.15.47/ ){
    #    open(OUTPUT,">log/$remote_addr") || die "write_to_file: open $remote_addr failed";
        #    print OUTPUT $$xml_str_p;
        #    close(OUTPUT) || die "write_to_file: close $remote_addr failed";
    #}  
}

sub send_log {
    return if(!$DEBUG);
    my $log_info = shift;
    my $time     = scalar localtime;
    open( HDW, ">>", $LOG_FILE )||die("Can not open file:$LOG_FILE\n");
    print HDW $time . " " . $log_info . "\n";
    close HDW;
}