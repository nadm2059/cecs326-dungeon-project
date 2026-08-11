# Stage 1: Build environment
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y build-essential gcc make

WORKDIR /app
COPY . .

# Link precompiled object file and compile targets
RUN cp dungeon_X86_64.o dungeon.o && make

# Stage 2: Runtime environment
FROM ubuntu:24.04
WORKDIR /app

# Copy compiled executables from the builder stage
COPY --from=builder /app/game /app/barbarian /app/wizard /app/rogue /app/

CMD ["./game"]
