/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 SEBASTIEN DERONNE
 *
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
 *
 * Author: Sebastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"

// This is a simple example in order to show how to configure an IEEE 802.11ac Wi-Fi network.
//
// It ouputs the UDP goodput for every VHT bitrate value, which depends on the MCS value (0 to 9, where 9 is
// forbidden when the channel width is 20 MHz), the channel width (20, 40, 80 or 160 MHz) and the guard interval (long
// or short). The PHY bitrate is constant over all the simulation run. The user can also specify the distance between
// the access point and the station: the larger the distance the smaller the goodput.
//
// The simulation assumes a single station in an infrastructure network:
//
//  STA     AP
//    *     *
//    |     |
//   n1     n2
//
//Packets in this simulation aren't marked with a QosTag so they are considered
//belonging to BestEffort Access Class (AC_BE).
//
// Modified by: Luis Sanabria-Russo <sanabriarusso@gmail.com>

using namespace ns3;

/*
My libs
*/
#include <assert.h>     /* assert */


struct my_struct // user-defined structure
{
  double rxBytes = 0;
};

NS_LOG_COMPONENT_DEFINE ("quick-vht-wifi");

static void
TraceRxPackets(struct my_struct *counter, Ptr<const Packet> p, const Address &a)
{
  counter->rxBytes += double(p->GetSize ());

  /*
  // Uncomment the multi-line section above to output to screen while executing

  std::cout << "[" << Simulator::Now () << "] Address: " <<
    InetSocketAddress::ConvertFrom(a).GetIpv4 ()
    << ", bytes: " << double(p->GetSize ()) << std::endl;
  */
}


int main (int argc, char *argv[])
{
  bool udp = true;
  double simulationTime = 10; //seconds
  double distance = 1.0; //meters
  
  uint32_t seed = -1;
  uint32_t gi = 0;
  uint32_t stations = 2;
  uint32_t mcs = 0;
  uint32_t channelWidth = 20;

  CommandLine cmd;
  cmd.AddValue ("channelWidth", "WiFi Channel width", channelWidth);
  cmd.AddValue ("mcs", "Modulation and Coding scheme", mcs);
  cmd.AddValue ("seed", "Random number seed", seed);
  cmd.AddValue ("stations", "Number of stations per AP", stations);
  cmd.AddValue ("distance", "Distance in meters between the station and the access point", distance);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("udp", "UDP if set to 1, TCP otherwise", udp);
  cmd.Parse (argc,argv);

  if (seed > 0)
    RngSeedManager::SetSeed (seed);

  for (uint32_t s = 1; s <= stations; s++)
    {
      struct my_struct counter; //counting the number of rx bytes
      std::cout << "-->Stations: " << s << ":" << std::endl;
      uint32_t payloadSize; //1500 byte IP packet
      if (udp)
        {
          payloadSize = 1472; //bytes
        }
      else
        {
          payloadSize = 1448; //bytes
          Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));
        }

        NodeContainer wifiStaNode;
        wifiStaNode.Create (s);
        NodeContainer wifiApNode;
        wifiApNode.Create (1);

        YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
        YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
        phy.SetChannel (channel.Create ());

        // Set guard interval
        phy.Set ("ShortGuardEnabled", BooleanValue (gi));

        WifiHelper wifi;
        wifi.SetStandard (WIFI_PHY_STANDARD_80211ac);
        WifiMacHelper mac;

        std::ostringstream oss;
        oss << "VhtMcs" << mcs;
        wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss.str ()),
                                      "ControlMode", StringValue (oss.str ()));

        Ssid ssid = Ssid ("ns3-80211ac");

        mac.SetType ("ns3::StaWifiMac",
                     "Ssid", SsidValue (ssid));

        NetDeviceContainer staDevice;
        staDevice = wifi.Install (phy, mac, wifiStaNode);

        mac.SetType ("ns3::ApWifiMac",
                     "Ssid", SsidValue (ssid));

        NetDeviceContainer apDevice;
        apDevice = wifi.Install (phy, mac, wifiApNode);

        // Set channel width
        Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (channelWidth));

        // mobility.
        MobilityHelper mobility;
        Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

        positionAlloc->Add (Vector (0.0, 0.0, 0.0));
        positionAlloc->Add (Vector (distance, 0.0, 0.0));
        mobility.SetPositionAllocator (positionAlloc);

        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

        mobility.Install (wifiApNode);
        mobility.Install (wifiStaNode);

        /* Internet stack*/
        InternetStackHelper stack;
        stack.Install (wifiApNode);
        stack.Install (wifiStaNode);

        Ipv4AddressHelper address;

        address.SetBase ("192.168.1.0", "255.255.255.0");
        Ipv4InterfaceContainer staNodeInterface;
        Ipv4InterfaceContainer apNodeInterface;

        staNodeInterface = address.Assign (staDevice);
        apNodeInterface = address.Assign (apDevice);

        /* Setting applications */
        ApplicationContainer serverApp, clientApp;
        if (udp)
          {
            //UDP sink server
            PacketSinkHelper myServer ("ns3::UdpSocketFactory", InetSocketAddress (apNodeInterface.GetAddress (0), 9));
            serverApp.Add (myServer.Install (wifiApNode.Get (0)));
            serverApp.Start (Seconds (0.0));
            serverApp.Stop (Seconds (simulationTime + 1));

            UdpClientHelper myClient (apNodeInterface.GetAddress (0), 9);
            myClient.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
            myClient.SetAttribute ("Interval", TimeValue (Time ("0.00001"))); //packets/s
            myClient.SetAttribute ("PacketSize", UintegerValue (payloadSize));

            clientApp = myClient.Install (wifiStaNode);
            clientApp.Start (Seconds (1.0));
            clientApp.Stop (Seconds (simulationTime + 1));
          }
        else
          {
            //TCP flow
            //needs to be implemented
            assert(false);
          }

        // Distributing routing tables
        Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
        Simulator::Stop (Seconds (simulationTime + 1));


        // Connecting trace sources
        Config::ConnectWithoutContext ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
          MakeBoundCallback(&TraceRxPackets, &counter));

        Simulator::Run ();
        if (udp)
          {
            //UDP
            assert (clientApp.GetN () == wifiStaNode.GetN ());
            /*BLA:
            1. Can you compute the number of bytes received at the Sink using the data
            containede my_struct?
            2. Can you compute the overall thorughput per a given number of stations?*/
          }
        else
          {
            //TCP
            //needs to be implemented
            assert(false);
          }
        Simulator::Destroy ();
    }
  return 0;
}
