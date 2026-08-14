# Stage 1: Build environment
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y build-essential gcc make

WORKDIR /app
COPY . .

# Link precompiled object file and compile targets
RUN cp dungeon_X86_64.o dungeon.o && make

# Stage 2: Runtime environment (Web & CLI support)
FROM ubuntu:24.04
WORKDIR /app

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime C libraries and Python/Flask for the web UI
RUN apt-get update && apt-get install -y --no-install-recommends \
    libc6 \
    python3 \
    python3-flask \
 && rm -rf /var/lib/apt/lists/*

# Copy compiled binaries from the builder stage
COPY --from=builder /app/game /app/barbarian /app/wizard /app/rogue /app/

# Copy web app files (ensures /app/templates/index.html structure is preserved)
COPY app.py /app/app.py
COPY templates /app/templates

EXPOSE 5000
ENV PORT=5000

# Default to starting the web server
CMD ["python3", "app.py"]
