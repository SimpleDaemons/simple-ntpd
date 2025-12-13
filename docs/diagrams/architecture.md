# Simple NTP Daemon - Architecture Diagrams

## System Architecture

```mermaid
graph TB
    subgraph "Application Layer"
        Main[main.cpp]
        Server[NtpServer]
    end

    subgraph "Core Layer"
        Connection[NtpConnection<br/>Client Connection Handler]
        Packet[NtpPacket<br/>NTP Message Processing]
    end

    subgraph "Configuration Layer"
        Config[NtpConfig<br/>Configuration Management]
    end

    subgraph "Utilities Layer"
        Logger[Logger<br/>Logging System]
        Platform[Platform<br/>OS Abstraction]
    end

    Main --> Server
    Server --> Connection
    Server --> Config
    Server --> Logger
    Server --> Platform

    Connection --> Packet
    Packet --> Logger
```

## Detailed System Architecture

```mermaid
graph TB
    subgraph "Client Layer"
        Client1[NTP Client 1]
        Client2[NTP Client 2]
        ClientN[NTP Client N]
    end

    subgraph "Network Layer"
        UDP[UDP Socket<br/>Port 123]
        IPv4[IPv4 Stack]
        IPv6[IPv6 Stack]
    end

    subgraph "Connection Management"
        ConnMgr[Connection Manager<br/>Client Connections]
        ConnPool[Connection Pool<br/>Reusable Connections]
    end

    subgraph "NTP Protocol Layer"
        PacketHandler[Packet Handler<br/>NTP Packet Processing]
        TimeCalc[Time Calculator<br/>Offset/Delay Calculation]
        StratumMgr[Stratum Manager<br/>Time Source Hierarchy]
    end

    subgraph "Time Sources"
        SystemClock[System Clock<br/>OS Time]
        HardwareClock[Hardware Clock<br/>RTC]
        UpstreamNTP[Upstream NTP Servers<br/>Stratum 1/2]
    end

    subgraph "Time Synchronization"
        SyncEngine[Sync Engine<br/>Clock Adjustment]
        Filter[Filter<br/>Outlier Detection]
        Select[Select<br/>Best Time Source]
    end

    subgraph "Utilities"
        Logger[Logger<br/>Structured Logging]
        Metrics[Metrics<br/>Time Statistics]
    end

    Client1 --> UDP
    Client2 --> UDP
    ClientN --> UDP

    UDP --> IPv4
    UDP --> IPv6

    IPv4 --> ConnMgr
    IPv6 --> ConnMgr

    ConnMgr --> ConnPool
    ConnPool --> PacketHandler

    PacketHandler --> TimeCalc
    PacketHandler --> StratumMgr

    StratumMgr --> SystemClock
    StratumMgr --> HardwareClock
    StratumMgr --> UpstreamNTP

    TimeCalc --> SyncEngine
    SyncEngine --> Filter
    Filter --> Select
    Select --> SystemClock

    PacketHandler --> Logger
    TimeCalc --> Metrics
```

## NTP Packet Exchange Flow

```mermaid
sequenceDiagram
    participant Client
    participant Server
    participant Connection
    participant Packet

    Client->>Server: NTP Request (UDP)
    Server->>Connection: Handle Connection
    Connection->>Packet: Parse NTP Packet
    Packet->>Packet: Validate Packet
    Packet->>Packet: Calculate Timestamp
    Packet->>Packet: Build Response
    Packet->>Connection: NTP Response Packet
    Connection->>Client: NTP Response (UDP)

    Note over Client,Server: Response includes server timestamp
```

## NTP Server Processing Flow

```mermaid
flowchart TD
    Start([UDP Packet Received]) --> Parse[Parse NTP Packet]
    Parse --> Validate{Valid Packet?}
    Validate -->|No| LogError[Log Error]
    Validate -->|Yes| CheckMode{Packet Mode?}

    CheckMode -->|Client Mode 3| ProcessRequest[Process Client Request]
    CheckMode -->|Server Mode 4| ProcessResponse[Process Server Response]
    CheckMode -->|Broadcast Mode 5| ProcessBroadcast[Process Broadcast]

    ProcessRequest --> GetTime[Get System Time]
    GetTime --> Calculate[Calculate Timestamps]
    Calculate --> BuildResponse[Build NTP Response]
    BuildResponse --> SendResponse[Send Response]

    ProcessResponse --> UpdateStats[Update Statistics]
    ProcessBroadcast --> UpdateStats
    SendResponse --> UpdateStats
    LogError --> End([End])
    UpdateStats --> End
```
