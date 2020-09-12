#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/stats-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FirstExerciseLogger");

int main(int argc, char* argv[]) {
  CommandLine cmd(__FILE__);
  cmd.Parse(argc, argv);

  Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));

  PointToPointHelper pointToPointRouter;
  pointToPointRouter.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  pointToPointRouter.SetChannelAttribute("Delay", StringValue("20ms"));

  PointToPointHelper pointToPointLeaf;
  pointToPointLeaf.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  pointToPointLeaf.SetChannelAttribute("Delay", StringValue("40ms"));

  PointToPointDumbbellHelper pointToPointDumbbell (
      2, pointToPointLeaf,
      2, pointToPointLeaf,
      pointToPointRouter
  );

  InternetStackHelper stack;
  pointToPointDumbbell.InstallStack(stack);
  pointToPointDumbbell.AssignIpv4Addresses(
      Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),
      Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),
      Ipv4AddressHelper("10.3.1.0", "255.255.255.0")
  );

  OnOffHelper firstSourceHelper( // Node 1
      "ns3::TcpSocketFactory",
      Address()
  );

  firstSourceHelper.SetConstantRate(DataRate("5Mbps"), 1024);

  AddressValue firstDestinationAddress(InetSocketAddress(pointToPointDumbbell.GetRightIpv4Address(0), 1000));

  firstSourceHelper.SetAttribute("Remote", firstDestinationAddress);

  OnOffHelper secondSourceHelper( // Node 2
      "ns3::UdpSocketFactory",
      Address()
  );

  secondSourceHelper.SetConstantRate(DataRate("5Mbps"), 1024);

  AddressValue secondDestinationAddress(InetSocketAddress(pointToPointDumbbell.GetRightIpv4Address(1), 1000));

  secondSourceHelper.SetAttribute("Remote", secondDestinationAddress);

  PacketSinkHelper firstDestinationHelper( // Node 3
      "ns3::TcpSocketFactory",
      Address()
  );

  firstDestinationHelper.SetAttribute("Local", firstDestinationAddress);

  PacketSinkHelper secondDestinationHelper( // Node 4
      "ns3::UdpSocketFactory",
      Address()
  );
 
  secondDestinationHelper.SetAttribute("Local", secondDestinationAddress);

  ApplicationContainer firstApplications;
  ApplicationContainer firstSinks;

  firstApplications.Add(
      firstSourceHelper.Install(pointToPointDumbbell.GetLeft(0))
  );
  firstSinks.Add(
      firstDestinationHelper.Install(pointToPointDumbbell.GetRight(0))
  );

  firstSinks.Start(Seconds(0.0));
  firstSinks.Stop(Seconds(40.0));

  firstApplications.Start(Seconds(1.0));
  firstApplications.Stop(Seconds(40.0));

  ApplicationContainer secondApplications;
  ApplicationContainer secondSinks;

  secondApplications.Add(
      secondSourceHelper.Install(pointToPointDumbbell.GetLeft(1))
  );
  secondSinks.Add(
      secondDestinationHelper.Install(pointToPointDumbbell.GetRight(1))
  );

  secondSinks.Start(Seconds(0.0));
  secondSinks.Stop(Seconds(40.0));

  secondApplications.Start(Seconds(20.0));
  secondApplications.Stop(Seconds(40.0));

  NodeContainer sinkNodes;
  sinkNodes.Add(pointToPointDumbbell.GetRight(0));
  sinkNodes.Add(pointToPointDumbbell.GetRight(1));

  pointToPointRouter.EnablePcap("Ex3.3", sinkNodes);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  Simulator::Stop(Seconds(1.1));
  Simulator::Run();

  GnuplotHelper plotHelper;

  plotHelper.ConfigurePlot(
    "congestion-window",
    "Tamanho da Janela de Congestionamento Vs. Tempo",
    "Tempo (Segundos)",
    "Tamanho da Janela de Congestionamento"
  );

  plotHelper.PlotProbe(
    "ns3::Uinteger32Probe",
    "/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/CongestionWindow",
    "Output",
    "Tamanho da Janela de Congestionamento",
    GnuplotAggregator::KEY_BELOW
  );

  Simulator::Schedule(
    Seconds(30.0),
    Config::Set,
    "/NodeList/3/ApplicationList/*/$ns3::OnOffApplication/DataRate",
    StringValue("10Mbps")
  );

  Simulator::Stop(Seconds(40.0));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}
