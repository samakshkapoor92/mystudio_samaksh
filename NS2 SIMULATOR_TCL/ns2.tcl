#TEAM-14 HW-4 SAMAKSH KAPOOR, JUNG-IN KOH
set ns [new Simulator]
set k 0    
  			
set R1 [$ns node] 
set R2 [$ns node]
set src1 [$ns node]
set src2 [$ns node]
set rcv1 [$ns node] 
set rcv2 [$ns node]

set TraceFile [open output.tr w] 
set i1 [open out1.tr w]
set i2 [open out2.tr w]
$ns trace-all $TraceFile
set dta 0
$ns color 1 Red
set dta1 0
$ns color 2 Blue


set NAM_FILE [open output.nam w]
$ns namtrace-all $NAM_FILE

global tcp_ver op

set tcp_ver [lindex $argv 0]

set op [lindex $argv 1]    

if {$tcp_ver == "SACK"} {set tcp_ver Sack1}

if {$tcp_ver == "VEGAS"} {set tcp_ver Vegas}

if {$op == "1"} {set latency 12.5ms}

if {$op == "2"} {set latency 20ms}

if {$op == "3"} {set latency 27.5ms}


$ns duplex-link $R1 $R2 1Mb 100ms DropTail 
$ns duplex-link $src1 $R1 10Mb 5ms DropTail
$ns duplex-link $R2 $rcv1 10Mb 5ms DropTail
$ns duplex-link $src2 $R1 10Mb $latency DropTail
$ns duplex-link $R2 $rcv2 10Mb $latency DropTail
$ns duplex-link-op $R2 $rcv1 orient right-up
$ns duplex-link-op $R2 $rcv2 orient right-down
$ns duplex-link-op $src1 $R1 orient left-down
$ns duplex-link-op $src2 $R1 orient left-up

$ns queue-limit $R1 $R2 40

set tcp_1 [new Agent/TCP/$tcp_ver]
$ns attach-agent $src1 $tcp_1

if {$tcp_ver == "Vegas" } {
	set sink_1 [new Agent/TCPSink]
}
if {$tcp_ver == "Sack1" } {
	set sink_1 [new Agent/TCPSink/Sack1]
}
$ns attach-agent $rcv1 $sink_1
$ns connect $tcp_1 $sink_1


set ftp_1 [new Application/FTP]
$ftp_1 attach-agent $tcp_1
$ftp_1 set type_ FTP
 
set tcp_2 [new Agent/TCP/$tcp_ver]
$ns attach-agent $src2 $tcp_2
if {$tcp_ver == "Vegas"} {set sink_2 [new Agent/TCPSink]
	
} else {set sink_2 [new Agent/TCPSink/Sack1]
}

$ns attach-agent $rcv2 $sink_2
$ns connect $tcp_2 $sink_2
$ns set fid_ 1
 
set ftp_2 [new Application/FTP]
$ftp_2 attach-agent $tcp_2
$ftp_2 set type_ FTP

proc rec_rd {} {	
	set time 1
        global sink_1 sink_2 i1 i2 k dta dta1
	set ns [Simulator instance]
	set bw_0 [$sink_1 set bytes_]
	set bw_1 [$sink_2 set bytes_]
	set k [expr $k + 1]	
	set now [$ns now]
	puts $i1 "$now [expr $bw_0/$time*8/1000000]"
	puts $i2 "$now [expr $bw_1/$time*8/1000000]"
	if { $k >= 100 } {set dta [expr $dta + $bw_0]}
	if { $k >= 100 } {set dta1 [expr $dta1 + $bw_1]}	
	$sink_1 set bytes_ 0
	$sink_2 set bytes_ 0

        $ns at [expr $now+$time] "rec_rd"
}
proc END {} {
		global ns TraceFile NAM_FILE i1 i2 thru_put1 dta thru_put2 dta1 k
		set thru_put1 [expr $dta*8/300]
		puts "Time : 300"
		puts "dta(bytes) : $dta"
		puts "Throughput for src1 : $thru_put1 bps"
		set thru_put2 [expr $dta1*8/300]
		puts "Time : 300"
		puts "dta(bytes) : $dta1"
		puts "Throughput for src2 : $thru_put2 bps"
		set k 0 
		$ns flush-trace
		close $TraceFile
		close $NAM_FILE
		close $i1
		close $i2
		exec nam output.nam &
		exit 0
}

$ns at 0.0 "rec_rd" 
$ns at 0.0 "$ftp_1 start"
$ns at 0.0 "$ftp_2 start"
$ns at 400.0 "$ftp_1 stop"
$ns at 400.0 "$ftp_2 stop"
$ns at 400.0 "END"
$ns run
