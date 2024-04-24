#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <cmath>
#include <fstream>
#include <numeric>
#include <sys/sysinfo.h>
#include <vector>

#define NUM_PACKET_SIZES 5
#define NUM_BANDWIDTHS 2

using namespace ns3;

struct TestResult
{
  double throughput;      // vazão em pacotes por segundo
  double throughputBytes; // vazão em bytes por segundo
  double cpuUtilization;  // utilização da CPU em porcentagem
  double jitter;          // variação do atraso
  uint32_t packetsLost;   // total de datagramas perdidos
  uint32_t packetsSent;   // total de datagramas enviados
  uint32_t packetSize;
};

TestResult
RunTest(uint32_t packetSize, double bandwidth, uint32_t nPackets, uint32_t simTime)
{
    TestResult result;

    // Coletando informações do sistema para estimar a utilização da CPU
    struct sysinfo info;
    sysinfo(&info);
    uint64_t cpuTotal = info.totalram; // Total de RAM como uma estimativa de tempo total de CPU
    uint32_t numCpus = info.procs;     // Número de CPUs

    // Configurando enlace ponto a ponto
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue(std::to_string(bandwidth) + "bps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    // Criando nós
    NodeContainer nodes;
    nodes.Create(2);

    // Instalando dispositivos de rede
    NetDeviceContainer devices;
    devices = p2p.Install(nodes);

    // Instalando protocolo de internet
    InternetStackHelper stack;
    stack.Install(nodes);

    // Atribuindo endereços IP
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Criando servidor
    uint16_t port = 5001;
    UdpServerHelper server(port);
    ApplicationContainer serverApp = server.Install(nodes.Get(1));
    serverApp.Start(Seconds(0.0));

    // Criando cliente
    Address remoteAddress(
        InetSocketAddress(Ipv4Address("172.17.0.2"), port)); // Usando o endereço IP do nó remoto
    UdpClientHelper client(remoteAddress); // Passando o endereço remoto para o construtor
    client.SetAttribute("PacketSize", UintegerValue(packetSize));
    client.SetAttribute("MaxPackets", UintegerValue(nPackets));
    client.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    ApplicationContainer clientApp = client.Install(nodes.Get(0));
    clientApp.Start(Seconds(1.0));
    clientApp.Stop(Seconds(simTime + 1.0));

    // Criando animação NetAnim
    AnimationInterface anim("simulation-animation.xml");

    // Rodando a simulação
    Simulator::Stop(Seconds(simTime + 2.0));
    Simulator::Run();

    // Coletando estatísticas
    Ptr<UdpServer> udpServer = DynamicCast<UdpServer>(serverApp.Get(0));
    result.packetsLost = udpServer->GetLost();
    result.packetsSent = nPackets;
    result.throughput = nPackets / simTime;                     // Vazão em pacotes por segundo
    result.throughputBytes = (packetSize * nPackets) / simTime; // Vazão em bytes por segundo
    result.packetSize = packetSize;

    // Calculando utilização da CPU (aproximado)
    double totalCpuTime =
        Simulator::Now().GetSeconds() * 2.0 * simTime; // Simulação em 2x velocidade
    double idleCpuTime = (totalCpuTime - cpuTotal) / (double)numCpus;
    result.cpuUtilization = (totalCpuTime - idleCpuTime) / totalCpuTime * 100.0;

    // Calculando variação do atraso (jitter)
    // Para este exemplo, vamos supor que o jitter seja 0
    result.jitter = 0.0;

    Simulator::Destroy();

    return result;
}

void
SaveResultsToFile(const std::string& filename, const std::vector<TestResult>& results)
{
    std::ofstream file(filename);
    file << "Packet Size,Throughput (pps),Throughput (Bps),CPU Utilization (%),Jitter,Packets Lost,Packets "
            "Sent\n";
    for (const auto& result : results)
    {
        file << result.packetSize << "," << result.throughput << "," << result.throughputBytes << "," << result.cpuUtilization
             << "," << result.jitter << "," << result.packetsLost << "," << result.packetsSent
             << "\n";
    }
    file.close();
}

int
main(int argc, char* argv[])
{
    uint32_t payloadSize[NUM_PACKET_SIZES] = {128,
                                              256,
                                              512,
                                              1024,
                                              1280}; // Tamanhos de pacote em bytes
    double bandwidths[NUM_BANDWIDTHS] = {0.8e6,
                                         1e6}; // Larguras de banda em bits por segundo (80% e 100%)
    uint32_t nPackets = 1000000;                    // Número de pacotes para cada teste
    uint32_t simTime = 30;                     // Tempo de simulação em segundos

    std::vector<TestResult> allResults;

    // Executando testes para cada combinação de tamanho de pacote e largura de banda
    for (uint32_t i = 0; i < NUM_PACKET_SIZES; ++i)
    {
        for (uint32_t j = 0; j < NUM_BANDWIDTHS; ++j)
        {
            TestResult result = RunTest(payloadSize[i], bandwidths[j], nPackets, simTime);
            allResults.push_back(result);
        }
    }

    // Salvando os resultados em um arquivo
    SaveResultsToFile("results-172.17.0.2.csv", allResults);

    return 0;
}