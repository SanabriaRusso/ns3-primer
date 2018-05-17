#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
 
using namespace ns3;
 
NS_LOG_COMPONENT_DEFINE ("quick-vht-wifi");
 
int main (int argc, char *argv[])
{
  // It is here, at the main function where the execution of the script will begin
 
  // Defining variables for the simulation
  bool udp = true;
  double simulationTime = 10; //seconds
  double distance = 1.0; //meters
  
  uint32_t seed = -1; 
  uint32_t gi = 0;
  uint32_t stations = 10;
  uint32_t mcs = 0;
  uint32_t channelWidth = 20;
  std::ofstream outputFile;
 
 
  /*
  We can also specify how the variables defined above can be passed as parameters
  to this script. As shown below, this is done through the use of the CommandLine class
  */
 
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
    // we define parameters for each of the stations in the simulation
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
    wifi.SetStandard (WIFI_PHY_STANDARD_80211ac); //define the WiFi Standard
    WifiMacHelper mac;
 
    std::ostringstream oss;
    oss << "VhtMcs" << mcs; wifi.SetRemoteStationManager 
      ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss.str ()), 
      "ControlMode", StringValue (oss.str ())); 
    Ssid ssid = Ssid ("ns3-80211ac"); // define the SSID of the network 
 
    mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid)); 
    NetDeviceContainer staDevice; 
    staDevice = wifi.Install (phy, mac, wifiStaNode); 
    mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid)); 
     
    NetDeviceContainer apDevice; 
    apDevice = wifi.Install (phy, mac, wifiApNode); 
 
    // Set channel width directly using Config::Set and the route to the Class's property 
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (channelWidth)); 
 
    // Configure the position of the nodes in the deployment 
    MobilityHelper mobility; 
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>(); 
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
 
    // Define an IPv4 address range
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
      /*
      Here we have define a single UDP traffic source as application. You can 
      see below the different parameters admitted by the application's object.
      */
       
      //UDP flow
      UdpServerHelper myServer (9);
      serverApp = myServer.Install (wifiApNode.Get (0));
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
      NS_ASSERT (false);
    }
 
    // Distributing routing tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
 
    // determine the end of the simulation in seconds
    Simulator::Stop (Seconds (simulationTime + 1));
    Simulator::Run ();
    Simulator::Destroy (); //clean everythin after each simulation
 
    double throughput = 0;
    if (udp)
    {
      //UDP
      NS_ASSERT (clientApp.GetN () == wifiStaNode.GetN ());
      uint32_t totalPacketsThrough = DynamicCast<UdpServer> (serverApp.Get (0))->GetReceived ();
      throughput = totalPacketsThrough * payloadSize * 8 / (simulationTime * 1000000.0); //Mbit/s
      std::cout << "--->Throughput for " << s << " stations: " << throughput << " Mbps" << std::endl;

      outputFile.open("data.txt", std::ofstream::out | std::ofstream::app);
      outputFile << s << " " << throughput << std::endl;
      outputFile.close();
    }
    else
    {
      //TCP
      //needs to be implemented
      NS_ASSERT (false);
    }
  }
  return 0;
}
