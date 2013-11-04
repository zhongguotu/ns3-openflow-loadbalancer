/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Network topology
//
//                clients
//                   |
//       ------------------------
//       |        Switch        |
//       ------------------------
//        |      |      |      |
//        s0     s1     s2     s3
//
//


#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/openflow-module.h"
#include "ns3/log.h"

#include "openflow-loadbalancer.h"
#include "openflow-controller.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("OpenFlowLoadBalancerSimulation");

bool verbose = false;
int searver_number = OF_DEFAULT_SEARVER_NUMBER;
oflb_type lb_type = OFLB_RANDOM;

bool
SetVerbose (std::string value)
{
  verbose = true;
  return true;
}

bool
SetSearverNumber (std::string value)
{
  try {
    searver_number = atoi (value.c_str ());
    return true;
    }
  catch (...) { return false; }
  return false;
}

bool
SetType (std::string value)
{
  try {
    if (value == "random") {
      lb_type = OFLB_RANDOM;
      return true;
    }
    if (value == "round-robin") {
      lb_type = OFLB_ROUND_ROBIN;
      return true;
    }
    if (value == "ip-hashing") {
      lb_type = OFLB_IP_HASHING;
      return true;
    }
  }
  catch (...) { return false; }
  return false;
}

int
main (int argc, char *argv[])
{
  #ifdef NS3_OPENFLOW
  //
  // Allow the user to override any of the defaults and the above Bind() at
  // run-time, via command-line arguments
  //
  CommandLine cmd;
  cmd.AddValue ("v", "Verbose (turns on logging).", MakeCallback (&SetVerbose));
  cmd.AddValue ("verbose", "Verbose (turns on logging).", MakeCallback (&SetVerbose));
  cmd.AddValue ("n", "Number of Searver behind the Load-Balancer.", MakeCallback (&SetSearverNumber));
  cmd.AddValue ("number", "Number of Searver behind the Load-Balancer.", MakeCallback (&SetSearverNumber));
  cmd.AddValue ("t", "Load Balancer Type.", MakeCallback ( &SetType));
  cmd.AddValue ("type", "Load Balancer Type.", MakeCallback ( &SetType));

  cmd.Parse (argc, argv);

  if (verbose)
    {
      LogComponentEnable ("OpenFlowLoadBalancerSimulation", LOG_LEVEL_INFO);
      LogComponentEnable ("OpenFlowInterface", LOG_LEVEL_INFO);
      LogComponentEnable ("OpenFlowSwitchNetDevice", LOG_LEVEL_INFO);
    }

  //
  // Explicitly create the nodes required by the topology (shown above).
  //
  NS_LOG_INFO ("Create searvers.");
  NS_LOG_INFO (searver_number);
  NodeContainer searvers;
  searvers.Create (searver_number);

  NodeContainer csmaSwitch;
  csmaSwitch.Create (1);

  NS_LOG_INFO ("Build Topology");
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

  // Create the csma links, from each terminal to the switch
  NetDeviceContainer searverDevices;
  NetDeviceContainer switchDevices;
  for (int i = 0; i < searver_number; i++)
    {
      NetDeviceContainer link = csma.Install (NodeContainer (searvers.Get (i), csmaSwitch));
      searverDevices.Add (link.Get (0));
      switchDevices.Add (link.Get (1));
    }

  // Create the switch netdevice, which will do the packet switching
  Ptr<Node> switchNode = csmaSwitch.Get (0);
  OpenFlowSwitchHelper swtch;

  switch (lb_type) {
  case OFLB_RANDOM: {
    NS_LOG_INFO ("Using Random Load Balancer.");
    Ptr<ns3::ofi::DropController> controller = CreateObject<ns3::ofi::DropController> ();
    swtch.Install (switchNode, switchDevices, controller);
    break;
  }
  case OFLB_ROUND_ROBIN:
    NS_LOG_INFO ("Using Round-Robin Load Balancer.");
    break;
  case OFLB_IP_HASHING:
    NS_LOG_INFO ("Using IP-Hashing Load Balancer.");
    break;
  default:
    break;
  }

  // Add internet stack to the terminals
  InternetStackHelper internet;
  internet.Install (searvers);

  // We've got the "hardware" in place.  Now we need to add IP addresses.
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  ipv4.Assign (searverDevices);

  NS_LOG_INFO ("Configure Tracing.");

  //
  // Configure tracing of all enqueue, dequeue, and NetDevice receive events.
  // Trace output will be sent to the file "openflow-switch.tr"
  //
  AsciiTraceHelper ascii;
  csma.EnableAsciiAll (ascii.CreateFileStream ("openflow-loadbalancer.tr"));

  //
  // Also configure some tcpdump traces; each interface will be traced.
  // The output files will be named:
  //     openflow-switch-<nodeId>-<interfaceId>.pcap
  // and can be read by the "tcpdump -r" command (use "-tt" option to
  // display timestamps correctly)
  //
  csma.EnablePcapAll ("openflow-loadbalancer", false);

  //
  // Now, do the actual simulation.
  //
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
  #else
  NS_LOG_INFO ("NS-3 OpenFlow is not enabled. Cannot run simulation.");
  #endif // NS3_OPENFLOW
}
