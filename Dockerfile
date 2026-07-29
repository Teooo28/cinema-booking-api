# Use an official Ubuntu image as the base operating system
FROM ubuntu:24.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary build tools and dependencies (C++ compiler, CMake, Git)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory inside the container
WORKDIR /app

# Copy the entire source code into the container's working directory
COPY . .

# Create the build directory, configure the project, and compile the main API server
RUN mkdir build && cd build && cmake .. && cmake --build . --target oop

# Expose the port on which the Crow API server listens
EXPOSE 8080

# Define the default command to execute when the container starts
CMD ["./build/oop"]