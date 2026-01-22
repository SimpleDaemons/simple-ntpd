# Observability Guide

This guide shows how to aggregate logs and scrape metrics from `simple-ntpd` in common environments.

## Configure Logging for Aggregation

Recommended settings for containerized deployments:

```ini
# simple-ntpd.conf
enable_console_logging = true   # emit to stdout
log_json = true                 # structured JSON for ingestion
# optionally keep a rotating file copy
log_file = /var/log/simple-ntpd/simple-ntpd.log
log_max_size_bytes = 10485760   # 10 MiB
log_max_files = 5
enable_syslog = false
```

On hosts (non-container), prefer syslog in addition to file rotation:

```ini
enable_syslog = true
log_file = /var/log/simple-ntpd/simple-ntpd.log
log_max_size_bytes = 10485760
log_max_files = 5
```

## Prometheus Metrics

A minimal Prometheus-style text endpoint is available via a one-shot command:

```bash
simple-ntpd metrics
```

For scrape integration, wrap the command in a script or export through a sidecar that execs the command and exposes it over HTTP.

## Fluent Bit (tail JSON logs)

Example Fluent Bit configuration to tail the rotating log file and parse JSON:

```ini
[SERVICE]
    Flush        1
    Daemon       Off

[INPUT]
    Name         tail
    Path         /var/log/simple-ntpd/simple-ntpd.log*
    Tag          simple_ntpd
    Parser       json
    Refresh_Interval 5
    Rotate_Wait  5

[PARSER]
    Name         json
    Format       json

[FILTER]
    Name         modify
    Match        simple_ntpd
    Rename       msg message

[OUTPUT]
    Name         stdout
    Match        simple_ntpd
```

To collect from stdout (containers), switch INPUT to `tail` of Docker JSON logs or use the Fluent Bit container input if available.

## Filebeat (ship JSON logs)

Example Filebeat input for JSON log files:

```yaml
filebeat.inputs:
  - type: filestream
    id: simple-ntpd-logs
    enabled: true
    paths:
      - /var/log/simple-ntpd/simple-ntpd.log*
    parsers:
      - ndjson:
          overwrite_keys: true
          add_error_key: true
          message_key: msg
    fields:
      service.name: simple-ntpd
    fields_under_root: true

output.console:
  pretty: true
```

## Kubernetes Notes

- Set `enable_console_logging=true` and `log_json=true` to emit structured logs to stdout.
- Use a DaemonSet (Fluent Bit/Filebeat) to ship container logs to your backend.
- For metrics, create a small sidecar that serves the output of `simple-ntpd metrics` over HTTP for Prometheus scraping.

## Validation Checklist

- Logs appear in your aggregation backend with fields `ts`, `level`, `thread`, and `msg`.
- Rotation does not break ingestion (patterns include `simple-ntpd.log*`).
- Metrics show increasing counters for requests and errors under load.
