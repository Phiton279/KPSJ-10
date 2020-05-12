set opt(chan)           Channel/WirelessChannel    
set opt(prop)           Propagation/TwoRayGround   
set opt(netif)          Phy/WirelessPhy            
set opt(mac)            Mac/802_11                 
set opt(ifq)            Queue/DropTail/PriQueue    
set opt(ll)             LL                         
set opt(ant)            Antenna/OmniAntenna        
set opt(ifqlen)         50                         
set opt(nn)             10                         
set opt(adhocRouting)   DSDV                       
set opt(cp)             ""                         
set opt(sc)             ""                         
set opt(x)              500                        
set opt(y)              500                        
set opt(stop)           20                         
set opt(cbr_interval)   0.005
set opt(cbr_packetsize) 1000
set opt(super_frame)    50

ns-random 0.0                    ;##########

set ns_   [new Simulator]

$ns_ node-config -addressType hierarchical
AddrParams set domain_num_ 2
lappend cluster_num 1 1
AddrParams set cluster_num_ $cluster_num
lappend eilastlevel $opt(nn) 1
AddrParams set nodes_num_ $eilastlevel

$ns_ use-newtrace
set tracefd [open traceout.ns w]
$ns_ trace-all $tracefd
set namtrace [open traceout.nam w]
$ns_ namtrace-all $namtrace

set topo   [new Topography]
$topo load_flatgrid $opt(x) $opt(y)

create-god $opt(nn)

set W(0) [$ns_ node 0.0.0]

$ns_ node-config -adhocRouting $opt(adhocRouting) \
                 -llType $opt(ll) \
                 -macType $opt(mac) \
                 -ifqType $opt(ifq) \
                 -ifqLen $opt(ifqlen) \
                 -antType $opt(ant) \
                 -propType $opt(prop) \
                 -phyType $opt(netif) \
                 -channelType $opt(chan) \
		 -topoInstance $topo \
                 -wiredRouting ON \
		 -agentTrace OFF \
                 -routerTrace OFF \
                 -macTrace ON

set waddr {1.0.0 1.0.1 1.0.2 1.0.3 1.0.4 1.0.5 1.0.6 1.0.7 1.0.8 1.0.9 1.0.10}

set BS(0) [$ns_ node [lindex $waddr 0]]
$BS(0) random-motion 0
$BS(0) set X_ 250.0
$BS(0) set Y_ 250.0
$BS(0) set Z_ 0.0

$ns_ node-config -wiredRouting OFF

for {set j 0} {$j < $opt(nn)} {incr j} {
    set node_($j) [ $ns_ node [lindex $waddr \
	    [expr $j+1]] ]
    $node_($j) base-station [AddrParams addr2id \
	    [$BS(0) node-addr]]
}


#####
[$BS(0) set mac_(0)] make-pc
[$BS(0) set mac_(0)] beaconperiod $opt(super_frame)
for {set k 0} {$k<$opt(nn)} {incr k} {
    [$BS(0) set mac_(0)] addSTA [expr $k+1] 1 0
}
#####


$ns_ duplex-link $W(0) $BS(0) 10Mb 2ms DropTail
$ns_ duplex-link-op $W(0) $BS(0) orient down

for {set i 0} {$i < $opt(nn)} {incr i} {
    set udp_($i) [new Agent/UDP]
    $ns_ attach-agent $node_([expr $i]) $udp_($i)
    set cbr_($i) [new Application/Traffic/CBR]
    $cbr_($i) attach-agent $udp_($i)
    $cbr_($i) set packetSize_ $opt(cbr_packetsize)
#    $udp_($i) set packetSize_ 2500                ;########
    $cbr_($i) set interval_ $opt(cbr_interval)
    set null_($i) [new Agent/Null] 
    $ns_ attach-agent $W(0) $null_($i)
    $ns_ connect $udp_($i) $null_($i)
    $ns_ at [expr 2*$i] "$cbr_($i) start"
}

for {set i 0} {$i < $opt(nn)} {incr i} {
    $ns_ initial_node_pos $node_($i) 20
    $node_($i) set X_ [expr 100 - 4*$i]
    $node_($i) set Y_ [expr 100 + 4*$i]
}

$W(0) set X_ 50
$W(0) set Y_ 70

for {set i 0} {$i < $opt(nn) } {incr i} {
    $ns_ at $opt(stop).0 "$node_($i) reset";
}
$ns_ at $opt(stop).0 "$BS(0) reset";

$ns_ at $opt(stop).0002 "puts \"NS EXITING...\" ; $ns_ halt"
$ns_ at $opt(stop).0001 "stop"

#for {set i 0} {$i<$opt(nn)} {incr i} {            ;########
#$ns_ at $opt(stop).0 "$node_($i) reset";          ;########
#}                                                 ;########

proc stop {} {
    global ns_ tracefd namtrace
    $ns_ flush-trace
    close $tracefd
    close $namtrace
}

puts "Starting Simulation..."
$ns_ run


