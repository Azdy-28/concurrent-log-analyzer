import argparse
import random
import sys
from datetime import datetime, timedelta

LEVELS = ["TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"]

LEVEL_WEIGHTS = [5, 20, 50, 15, 8, 2]
MODULES = ["auth", "payments", "search", "cache", "gateway", "db", "scheduler", "notifications"]
MESSAGES = [
    "request completed in {ms}ms",
    "token validation failed for user {uid}",
    "cache miss for key shard-{uid}",
    "connection pool exhausted, retrying",
    "received heartbeat from node-{uid}",
    "query took {ms}ms, exceeds threshold",
    "retrying operation, attempt {attempt}",
    "user {uid} session started",
    "flushing buffer of {bytes} bytes",
    "job {uid} scheduled for next window",
]


def gen_line(ts: datetime) -> str:
    level = random.choices(LEVELS, weights=LEVEL_WEIGHTS, k=1)[0]
    module = random.choice(MODULES)
    template = random.choice(MESSAGES)
    msg = template.format(ms=random.randint(1, 4000), uid=random.randint(1000, 99999),
                           attempt=random.randint(1, 5), bytes=random.randint(64, 65536))
    return f"{ts.isoformat(timespec='milliseconds')}Z {level} {module}: {msg}\n"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output_path")
    parser.add_argument("target_size_mb", type=float)
    parser.add_argument("--seed", type=int, default=42)
    args = parser.parse_args()

    random.seed(args.seed)
    target_bytes = int(args.target_size_mb * 1024 * 1024)
    ts = datetime(2026, 7, 18, 0, 0, 0)
    written = 0

    with open(args.output_path, "w", buffering=1024 * 1024) as f:
        while written < target_bytes:
            chunk_lines = []
            for _ in range(5000):
                ts += timedelta(milliseconds=random.randint(1, 50))
                chunk_lines.append(gen_line(ts))
            chunk = "".join(chunk_lines)
            f.write(chunk)
            written += len(chunk)
            print(f"\r{written / (1024*1024):.1f} / {args.target_size_mb:.1f} MB", end="", file=sys.stderr)

    print(f"\nWrote {written / (1024*1024):.2f} MB to {args.output_path}", file=sys.stderr)


if __name__ == "__main__":
    main()
