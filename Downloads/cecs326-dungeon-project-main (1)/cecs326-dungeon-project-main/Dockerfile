# Stage 1: Build environment
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y build-essential gcc make

WORKDIR /app
COPY . .

# Link precompiled object file and compile targets
RUN cp dungeon_X86_64.o dungeon.o && make[cite: 1]

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

# Copy compiled executables from the builder stage
COPY --from=builder /app/game /app/barbarian /app/wizard /app/rogue /app/[cite: 1]

# Copy web server code and UI templates
COPY app.py /app/
COPY templates/ /app/templates/

EXPOSE 5000
ENV PORT=5000

# Default to starting the web server (ideal for Render & Web services)
CMD ["python3", "app.py"]