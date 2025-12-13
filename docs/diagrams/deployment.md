# Simple NTP Daemon - Deployment Diagrams

## Basic Deployment Architecture

```mermaid
graph TB
    subgraph "Client Network"
        Client1[NTP Client 1]
        Client2[NTP Client 2]
        ClientN[NTP Client N]
    end

    subgraph "NTP Server"
        Server[simple-ntpd<br/>Main Process]
        Config[/etc/simple-ntpd/<br/>Configuration]
        Logs[/var/log/simple-ntpd/<br/>Log Files]
    end

    subgraph "Time Sources"
        Upstream1[Upstream NTP Server 1<br/>pool.ntp.org]
        Upstream2[Upstream NTP Server 2<br/>time.google.com]
        SystemClock[System Clock<br/>OS Time]
    end

    subgraph "System Services"
        Systemd[systemd<br/>Service Manager]
    end

    Client1 --> Server
    Client2 --> Server
    ClientN --> Server

    Systemd --> Server
    Systemd --> Config

    Server --> Config
    Server --> Logs
    Server --> Upstream1
    Server --> Upstream2
    Server --> SystemClock
```

## Stratum Hierarchy Deployment

```mermaid
graph TB
    subgraph "Stratum 0"
        GPS[GPS Time Source]
        AtomicClock[Atomic Clock]
    end

    subgraph "Stratum 1"
        Stratum1_1[NTP Server 1<br/>Stratum 1]
        Stratum1_2[NTP Server 2<br/>Stratum 1]
    end

    subgraph "Stratum 2"
        Stratum2_1[simple-ntpd<br/>Stratum 2 Server 1]
        Stratum2_2[simple-ntpd<br/>Stratum 2 Server 2]
    end

    subgraph "Stratum 3"
        Stratum3_1[simple-ntpd<br/>Stratum 3 Server 1]
        Stratum3_2[simple-ntpd<br/>Stratum 3 Server 2]
    end

    subgraph "Clients"
        Client1[NTP Client 1]
        Client2[NTP Client 2]
        ClientN[NTP Client N]
    end

    GPS --> Stratum1_1
    AtomicClock --> Stratum1_2

    Stratum1_1 --> Stratum2_1
    Stratum1_2 --> Stratum2_2

    Stratum2_1 --> Stratum3_1
    Stratum2_2 --> Stratum3_2

    Stratum3_1 --> Client1
    Stratum3_2 --> Client2
    Stratum3_1 --> ClientN
    Stratum3_2 --> ClientN
```

## High Availability NTP Deployment

```mermaid
graph TB
    subgraph "Load Balancer"
        LB[Load Balancer<br/>NTP Query Distribution]
    end

    subgraph "NTP Server Pool"
        Server1[simple-ntpd<br/>Server 1]
        Server2[simple-ntpd<br/>Server 2]
        Server3[simple-ntpd<br/>Server 3]
    end

    subgraph "Time Sources"
        Upstream1[Upstream NTP 1]
        Upstream2[Upstream NTP 2]
        Upstream3[Upstream NTP 3]
    end

    subgraph "Clients"
        Client1[Client 1]
        Client2[Client 2]
        ClientN[Client N]
    end

    Client1 --> LB
    Client2 --> LB
    ClientN --> LB

    LB --> Server1
    LB --> Server2
    LB --> Server3

    Server1 --> Upstream1
    Server1 --> Upstream2
    Server2 --> Upstream2
    Server2 --> Upstream3
    Server3 --> Upstream1
    Server3 --> Upstream3
```
